mod feed;
mod package;
#[cfg(test)]
mod tests;

use feed::{invalid, Update};
use fs2::FileExt;
use sha2::{Digest, Sha256};
use std::{
    env,
    ffi::OsStr,
    fs::{self, File, OpenOptions},
    io::{self, Read, Write},
    path::{Path, PathBuf},
    process::{Command, Stdio},
    thread,
    time::{Duration, Instant},
};

fn status(session: &Path, value: &str) -> io::Result<()> {
    package::atomic_write(&session.join("status"), value.as_bytes())
}
fn workdir(root: &Path) -> PathBuf {
    if cfg!(target_os = "macos") {
        root.with_file_name(format!(
            ".{}.scu-update",
            root.file_name().unwrap_or_default().to_string_lossy()
        ))
    } else {
        root.join(".scu-update")
    }
}
fn locked(root: &Path) -> io::Result<(File, PathBuf)> {
    let w = workdir(root);
    if w.try_exists()? && fs::symlink_metadata(&w)?.is_symlink() {
        return Err(invalid("Linked update work directory"));
    }
    fs::create_dir_all(&w)?;
    let lock = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(false)
        .open(w.join("lock"))?;
    lock.try_lock_exclusive()?;
    let pending = w.join("transaction");
    if pending.try_exists()? {
        if !pending.join("owned-by-scu").is_file() {
            return Err(invalid("Unrecognized work directory"));
        }
        package::rollback(root, &pending, cfg!(target_os = "macos"))?;
        fs::remove_dir_all(&pending)?;
    }
    Ok((lock, pending))
}
fn recover(root: &Path) -> io::Result<()> {
    if workdir(root).join("transaction/journal").is_file() {
        let _guard = locked(root)?;
    }
    Ok(())
}

fn download(u: &Update, dest: &Path, session: &Path) -> io::Result<()> {
    let mut child = feed::fetch(&u.url, u.size, "240")?;
    let received = (|| {
        let mut input = child.stdout.take().unwrap();
        let mut file = File::create_new(dest)?;
        let mut seen = 0u64;
        let mut h = Sha256::new();
        let mut buf = [0u8; 65536];
        let mut last = Instant::now();
        loop {
            let n = input.read(&mut buf)?;
            if n == 0 {
                break;
            }
            seen += n as u64;
            if seen > u.size {
                return Err(invalid("Oversized download"));
            }
            h.update(&buf[..n]);
            file.write_all(&buf[..n])?;
            if last.elapsed() >= Duration::from_millis(250) {
                if session.join("stop").exists() {
                    return Err(invalid("Stopped"));
                }
                status(session, &format!("downloading {}", seen * 100 / u.size))?;
                last = Instant::now();
            }
        }
        if seen != u.size || format!("{:x}", h.finalize()) != u.sha {
            return Err(invalid("Download checksum mismatch"));
        }
        Ok(())
    })();
    if received.is_err() {
        let _ = child.kill();
    }
    let success = child.wait()?.success();
    received?;
    if !success {
        return Err(invalid("Download interrupted"));
    }
    Ok(())
}

fn alive(pid: u32) -> bool {
    #[cfg(unix)]
    {
        unsafe {
            libc::kill(pid as i32, 0) == 0
                || io::Error::last_os_error().raw_os_error() == Some(libc::EPERM)
        }
    }
    #[cfg(windows)]
    {
        use windows_sys::Win32::{
            Foundation::{CloseHandle, WAIT_TIMEOUT},
            System::Threading::{OpenProcess, WaitForSingleObject, PROCESS_SYNCHRONIZE},
        };
        unsafe {
            let handle = OpenProcess(PROCESS_SYNCHRONIZE, 0, pid);
            if handle.is_null() {
                return false;
            }
            let yes = WaitForSingleObject(handle, 0) == WAIT_TIMEOUT;
            CloseHandle(handle);
            yes
        }
    }
}
fn restart(root: &Path, data: &Path, cwd: &Path) -> io::Result<()> {
    let exe = if cfg!(windows) {
        root.join("SonicCompilationUltra.exe")
    } else if cfg!(target_os = "macos") {
        root.join("Contents/MacOS/SonicCompilationUltra")
    } else {
        root.join("SonicCompilationUltra.sh")
    };
    let mut cmd = Command::new(exe);
    cmd.current_dir(cwd)
        .arg("--menu")
        .arg("--no-autoresume")
        .arg("--data-dir")
        .arg(data)
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null());
    cmd.spawn().map(|_| ())
}

fn install(
    current: &str,
    chosen: &str,
    session: &Path,
    root: &Path,
    data: &Path,
    cwd: &Path,
    pid: u32,
) -> io::Result<()> {
    let u = feed::latest(current)?
        .filter(|u| u.version == chosen)
        .ok_or_else(|| invalid("Release changed"))?;
    let mac = cfg!(target_os = "macos");
    let (_lock, work) = locked(root)?;
    fs::create_dir(&work)?;
    fs::write(work.join("owned-by-scu"), b"1")?;
    let before_close = (|| {
        let old = package::prior(root, mac)?;
        let archive = work.join(&u.name);
        let newdir = work.join("new");
        fs::create_dir(&newdir)?;
        status(session, "downloading 0")?;
        download(&u, &archive, session)?;
        status(session, "verifying")?;
        let new = package::stage(&archive, &newdir, &u, feed::target())?;
        fs::remove_file(archive)?;
        let ops = package::prepare(root, &work, &old, &new, mac)?;
        // Read-only installations must fail while the library is still open.
        let mut dirs = std::collections::BTreeSet::new();
        for name in &new {
            let p = package::safe_dest(root, name, mac)?;
            let mut parent = p.parent().unwrap();
            while !parent.is_dir() {
                parent = parent
                    .parent()
                    .ok_or_else(|| invalid("Invalid destination"))?;
            }
            dirs.insert(parent.to_path_buf());
        }
        for parent in dirs {
            tempfile::NamedTempFile::new_in(parent)?;
        }
        if session.join("stop").exists() || !alive(pid) {
            return Err(invalid("Application closed"));
        }
        status(session, "ready")?;
        let deadline = Instant::now() + Duration::from_secs(45);
        loop {
            if session.join("closed").exists() && !alive(pid) {
                break;
            }
            if session.join("stop").exists() || Instant::now() > deadline {
                return Err(invalid("Restart timed out"));
            }
            thread::sleep(Duration::from_millis(100));
        }
        Ok(ops)
    })();
    let ops = match before_close {
        Ok(ops) => ops,
        Err(e) => {
            let _ = fs::remove_dir_all(&work);
            return Err(e);
        }
    };
    // Leave the journal and backups available until the replacement can launch.
    let result = package::apply(root, &work, &ops, mac).and_then(|_| restart(root, data, cwd));
    if let Err(err) = result {
        if !work.join("journal").exists() {
            package::atomic_write(&work.join("journal"), &serde_json::to_vec(&ops)?)?;
        }
        package::rollback(root, &work, mac)?;
        let _ = fs::write(data.join("update-install-error"), b"1");
        restart(root, data, cwd)?;
        fs::remove_dir_all(work)?;
        return Err(err);
    }
    fs::remove_dir_all(work)?;
    Ok(())
}

fn main() {
    let args: Vec<_> = env::args_os().skip(1).collect();
    if args.len() < 4 {
        std::process::exit(2);
    }
    let session = Path::new(&args[2]);
    let root = Path::new(&args[3]);
    let current = args[1].to_str().unwrap_or("");
    let check = args[0] == OsStr::new("check");
    let result = if check && args.len() == 4 {
        recover(root)
            .and_then(|_| feed::latest(current))
            .and_then(|update| {
                status(
                    session,
                    &update
                        .map_or_else(|| "current".into(), |u| format!("available {}", u.version)),
                )
            })
    } else if args[0] == OsStr::new("install") && args.len() == 8 {
        let chosen = args[4].to_str().unwrap_or("");
        args[7]
            .to_str()
            .and_then(|s| s.parse::<u32>().ok())
            .filter(|p| *p > 0)
            .ok_or_else(|| invalid("Invalid calling process"))
            .and_then(|pid| {
                install(
                    current,
                    chosen,
                    session,
                    root,
                    Path::new(&args[5]),
                    Path::new(&args[6]),
                    pid,
                )
            })
    } else {
        Err(invalid("Invalid command"))
    };
    if let Err(e) = result {
        let state = if check {
            "error check"
        } else if e.kind() == io::ErrorKind::PermissionDenied {
            "error permission"
        } else {
            "error install"
        };
        let _ = status(session, state);
        eprintln!("{e}");
        std::process::exit(1);
    }
}
