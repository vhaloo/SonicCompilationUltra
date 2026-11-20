use serde::Deserialize;
use std::{
    io,
    path::PathBuf,
    process::{Child, Command, Stdio},
};

pub const REPO: &str = "vhaloo/SonicCompilationUltra";
pub const FEED: &str =
    "https://api.github.com/repos/vhaloo/SonicCompilationUltra/releases?per_page=12";
pub const LIMIT: u64 = 160 * 1024 * 1024;
pub fn invalid(msg: &str) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, msg)
}

pub fn version(s: &str) -> Option<[u32; 3]> {
    let mut parts = s.split('.');
    let mut out = [0; 3];
    for n in &mut out {
        let p = parts.next()?;
        if p.is_empty()
            || p.len() > 1 && p.starts_with('0')
            || !p.bytes().all(|c| c.is_ascii_digit())
        {
            return None;
        }
        *n = p.parse().ok()?;
    }
    parts.next().is_none().then_some(out)
}

pub fn target() -> &'static str {
    match (std::env::consts::OS, std::env::consts::ARCH) {
        ("windows", "x86_64") => "windows-x64.zip",
        ("linux", "x86_64") => "linux-x64.tar.gz",
        ("macos", "x86_64") => "macos-x64.tar.gz",
        ("macos", "aarch64") => "macos-arm64.tar.gz",
        _ => "unsupported",
    }
}

#[derive(Deserialize)]
struct Release {
    tag_name: String,
    draft: bool,
    assets: Vec<Asset>,
}
#[derive(Deserialize)]
struct Asset {
    name: String,
    size: u64,
    digest: Option<String>,
    browser_download_url: String,
    state: String,
}
#[derive(Debug)]
pub struct Update {
    pub version: String,
    pub name: String,
    pub url: String,
    pub size: u64,
    pub sha: String,
}

pub fn select(data: &[u8], installed: &str, target: &str) -> io::Result<Option<Update>> {
    let current = version(installed).ok_or_else(|| invalid("Invalid installed version"))?;
    let releases: Vec<Release> = serde_json::from_slice(data)?;
    let mut best = current;
    let mut result = None;
    for rel in releases {
        if rel.draft {
            continue;
        }
        let label = rel
            .tag_name
            .strip_prefix("runtime-preview-")
            .or_else(|| rel.tag_name.strip_prefix('v'))
            .unwrap_or(&rel.tag_name);
        let Some(v) = version(label) else { continue };
        if v <= best {
            continue;
        }
        let name = format!("SonicCompilationUltra-{label}-{target}");
        let url = format!(
            "https://github.com/{REPO}/releases/download/{}/{name}",
            rel.tag_name
        );
        for a in rel.assets {
            if a.name != name
                || a.state != "uploaded"
                || a.browser_download_url != url
                || !(1..=LIMIT).contains(&a.size)
            {
                continue;
            }
            let Some(hash) = a.digest.as_deref().and_then(|s| s.strip_prefix("sha256:")) else {
                continue;
            };
            if hash.len() != 64 || !hash.bytes().all(|c| c.is_ascii_hexdigit()) {
                continue;
            }
            result = Some(Update {
                version: label.to_string(),
                name,
                url,
                size: a.size,
                sha: hash.to_ascii_lowercase(),
            });
            best = v;
            break;
        }
    }
    Ok(result)
}

// Use the platform's TLS trust store and a system executable, never a program
// from the game folder or a user's current working directory.
pub fn fetch(url: &str, size: u64, seconds: &str) -> io::Result<Child> {
    #[cfg(windows)]
    let binary = {
        use std::os::windows::ffi::OsStringExt;
        #[link(name = "kernel32")]
        extern "system" {
            fn GetSystemDirectoryW(buf: *mut u16, n: u32) -> u32;
        }
        let mut b = [0u16; 32768];
        let n = unsafe { GetSystemDirectoryW(b.as_mut_ptr(), b.len() as u32) } as usize;
        if n == 0 || n >= b.len() {
            return Err(io::Error::last_os_error());
        }
        PathBuf::from(std::ffi::OsString::from_wide(&b[..n])).join("curl.exe")
    };
    #[cfg(unix)]
    let binary = PathBuf::from("/usr/bin/curl");
    let mut cmd = Command::new(binary);
    cmd.args([
        "-q",
        "--silent",
        "--show-error",
        "--fail",
        "--location",
        "--proto",
        "=https",
        "--proto-redir",
        "=https",
        "--max-redirs",
        "5",
        "--connect-timeout",
        "6",
        "--max-time",
        seconds,
        "--max-filesize",
        &size.to_string(),
        "--user-agent",
        "SonicCompilationUltra-Updater",
        "--header",
        "Accept: application/vnd.github+json",
        "--",
        url,
    ])
    .stdin(Stdio::null())
    .stdout(Stdio::piped())
    .stderr(Stdio::null());
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        cmd.creation_flags(0x08000000);
    }
    cmd.spawn()
}

pub fn latest(current: &str) -> io::Result<Option<Update>> {
    use std::io::Read;
    const MAX: u64 = 2 * 1024 * 1024;
    let mut child = fetch(FEED, MAX, "15")?;
    let mut data = Vec::new();
    let result = child
        .stdout
        .take()
        .unwrap()
        .take(MAX + 1)
        .read_to_end(&mut data);
    if result.is_err() || data.len() as u64 > MAX {
        let _ = child.kill();
    }
    if !child.wait()?.success() || data.len() as u64 > MAX {
        return Err(invalid("Release check unavailable"));
    }
    result?;
    select(&data, current, target())
}
