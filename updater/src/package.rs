use crate::feed::{invalid, Update, LIMIT};
use flate2::read::GzDecoder;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeMap, BTreeSet},
    fs::{self, File},
    io::{self, Read, Write},
    path::{Component, Path, PathBuf},
};
use tempfile::NamedTempFile;

#[derive(Deserialize)]
struct Manifest {
    version: String,
    platform: String,
    architecture: String,
    game_data_included: bool,
    files: Vec<Entry>,
}
#[derive(Deserialize)]
struct Entry {
    path: String,
    bytes: u64,
    sha256: String,
}

fn relative(name: &str) -> io::Result<&Path> {
    if name.is_empty()
        || name.len() > 450
        || name.starts_with('/')
        || name.contains(['\\', ':', '\0'])
        || name.split('/').any(|s| {
            s.is_empty()
                || s == "."
                || s == ".."
                || s.ends_with(['.', ' '])
                || s.bytes().any(|c| c < 32)
                || matches!(
                    s.split('.')
                        .next()
                        .unwrap_or("")
                        .to_ascii_uppercase()
                        .as_str(),
                    "AUX" | "CON" | "NUL" | "PRN" | "COM1" | "LPT1"
                )
        })
        || !Path::new(name)
            .components()
            .all(|c| matches!(c, Component::Normal(_)))
    {
        return Err(invalid("Invalid package path"));
    }
    Ok(Path::new(name))
}

fn owned(name: &str, mac: bool) -> bool {
    if relative(name).is_err() {
        return false;
    }
    if mac {
        return matches!(
            name,
            "Contents/Info.plist"
                | "Contents/MacOS/SonicCompilationUltra"
                | "Contents/MacOS/scu-update"
                | "Contents/Resources/SCU.icns"
                | "Contents/Resources/SCUUpdateFiles.txt"
                | "Contents/_CodeSignature/CodeResources"
        ) || name.starts_with("Contents/Resources/assets/")
            || name.starts_with("Contents/Frameworks/");
    }
    matches!(
        name,
        "SonicCompilationUltra.exe"
            | "SonicCompilationUltra"
            | "scu-update.exe"
            | "scu-update"
            | "SDL2.dll"
            | "vcomp140.dll"
            | "msvcp140.dll"
            | "vcruntime140.dll"
            | "vcruntime140_1.dll"
            | "libSDL2-2.0.so.0"
            | "SonicCompilationUltra.sh"
            | "MANIFEST.json"
            | "LINUX_REQUIREMENTS.txt"
            | "LICENSE"
            | "README.md"
            | "THIRD_PARTY_NOTICES.md"
            | "CHANGELOG.md"
    ) || ["assets/", "licenses/", "docs/"]
        .iter()
        .any(|p| name.starts_with(p))
}
pub fn hash(path: &Path) -> io::Result<String> {
    let mut h = Sha256::new();
    io::copy(&mut File::open(path)?, &mut h)?;
    Ok(format!("{:x}", h.finalize()))
}

fn payload_path<'a>(path: &'a str, mac: bool) -> Option<&'a str> {
    if mac {
        path.strip_prefix("Sonic Compilation Ultra.app/")
    } else {
        Some(path)
    }
}
pub fn stage(archive: &Path, dir: &Path, u: &Update, target: &str) -> io::Result<BTreeSet<String>> {
    let (platform, remainder) = target
        .split_once('-')
        .ok_or_else(|| invalid("Unknown platform"))?;
    let arch = remainder.split('.').next().unwrap();
    let mac = platform == "macos";
    let prefix = format!("SonicCompilationUltra-{}-{}-{}/", u.version, platform, arch);
    let mut extracted = BTreeMap::new();
    let mut manifest = Vec::new();
    let mut total = 0u64;
    let mut put = |path: &str, size: u64, mode: u32, input: &mut dyn Read| -> io::Result<()> {
        let name = path
            .strip_prefix(&prefix)
            .ok_or_else(|| invalid("Unexpected package root"))?;
        relative(name)?;
        total = total
            .checked_add(size)
            .ok_or_else(|| invalid("Package size overflow"))?;
        if size > LIMIT
            || total > 3 * LIMIT
            || extracted.len() >= 4096
            || extracted.contains_key(name)
        {
            return Err(invalid("Invalid package entries"));
        }
        let mut digest = Sha256::new();
        if name == "MANIFEST.json" {
            if size > 2 * 1024 * 1024 || !manifest.is_empty() {
                return Err(invalid("Invalid manifest size"));
            }
            input.take(size + 1).read_to_end(&mut manifest)?;
            if manifest.len() as u64 != size {
                return Err(invalid("Truncated manifest"));
            }
            return Ok(());
        }
        let mut dest = None;
        if let Some(local) = payload_path(name, mac) {
            if !owned(local, mac) {
                return Err(invalid("File outside the program's managed paths"));
            }
            let dest_path = dir.join(local);
            fs::create_dir_all(dest_path.parent().unwrap())?;
            let f = File::create_new(&dest_path)?;
            #[cfg(unix)]
            {
                use std::os::unix::fs::PermissionsExt;
                f.set_permissions(fs::Permissions::from_mode(if mode & 0o111 != 0 {
                    0o755
                } else {
                    0o644
                }))?;
            }
            let _ = mode;
            dest = Some(f);
        }
        let mut seen = 0;
        let mut buf = [0u8; 65536];
        loop {
            let n = input.read(&mut buf)?;
            if n == 0 {
                break;
            }
            seen += n as u64;
            if seen > size {
                return Err(invalid("Oversized member"));
            }
            digest.update(&buf[..n]);
            if let Some(f) = dest.as_mut() {
                f.write_all(&buf[..n])?;
            }
        }
        if seen != size {
            return Err(invalid("Truncated member"));
        }
        extracted.insert(name.to_string(), (size, format!("{:x}", digest.finalize())));
        Ok(())
    };
    let file = File::open(archive)?;
    if target.ends_with(".zip") {
        let mut zip = zip::ZipArchive::new(file)?;
        if zip.len() > 4096 {
            return Err(invalid("Too many members"));
        }
        for i in 0..zip.len() {
            let mut entry = zip.by_index(i)?;
            if entry.is_dir() {
                continue;
            }
            let mode = entry.unix_mode().unwrap_or(0o644);
            if !matches!(mode & 0o170000, 0 | 0o100000) {
                return Err(invalid("Non-regular ZIP member"));
            }
            put(&entry.name().to_string(), entry.size(), mode, &mut entry)?;
        }
    } else {
        for entry in tar::Archive::new(GzDecoder::new(file)).entries()? {
            let mut e = entry?;
            if e.header().entry_type().is_dir() {
                continue;
            }
            if !e.header().entry_type().is_file() {
                return Err(invalid("Non-regular tar member"));
            }
            let n = e
                .path()?
                .to_str()
                .ok_or_else(|| invalid("Invalid filename"))?
                .to_owned();
            put(&n, e.size(), e.header().mode()?, &mut e)?;
        }
    }
    let m: Manifest = serde_json::from_slice(&manifest)?;
    if m.version != u.version
        || m.platform != platform
        || m.architecture != arch
        || m.game_data_included
        || m.files.len() != extracted.len()
    {
        return Err(invalid("Package identity mismatch"));
    }
    let mut result = BTreeSet::new();
    for e in m.files {
        if extracted.remove(&e.path) != Some((e.bytes, e.sha256)) {
            return Err(invalid("Member checksum mismatch"));
        }
        if let Some(local) = payload_path(&e.path, mac) {
            result.insert(local.to_string());
        }
    }
    if !mac {
        fs::write(dir.join("MANIFEST.json"), manifest)?;
        result.insert("MANIFEST.json".into());
    }
    let required = if mac {
        "Contents/MacOS/SonicCompilationUltra"
    } else if platform == "windows" {
        "SonicCompilationUltra.exe"
    } else {
        "SonicCompilationUltra.sh"
    };
    if !result.contains(required) {
        return Err(invalid("Required launcher missing"));
    }
    Ok(result)
}

pub fn prior(root: &Path, mac: bool) -> io::Result<BTreeSet<String>> {
    if mac {
        let names = fs::read_to_string(root.join("Contents/Resources/SCUUpdateFiles.txt"))?;
        return Ok(names
            .lines()
            .filter(|s| owned(s, true))
            .map(str::to_owned)
            .collect());
    }
    let m: Manifest = serde_json::from_reader(File::open(root.join("MANIFEST.json"))?)?;
    let mut names: BTreeSet<_> = m
        .files
        .into_iter()
        .map(|e| e.path)
        .filter(|s| owned(s, false))
        .collect();
    names.insert("MANIFEST.json".into());
    Ok(names)
}

fn is_link(meta: &fs::Metadata) -> bool {
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        if meta.file_attributes() & 0x400 != 0 {
            return true;
        }
    }
    meta.is_symlink()
}
pub fn safe_dest(root: &Path, name: &str, mac: bool) -> io::Result<PathBuf> {
    if !owned(name, mac) {
        return Err(invalid("Unmanaged path"));
    }
    let mut p = root.to_path_buf();
    for part in relative(name)?.components() {
        p.push(part);
        match fs::symlink_metadata(&p) {
            Ok(m) if is_link(&m) => return Err(invalid("Linked installation path")),
            Ok(_) => (),
            Err(e) if e.kind() == io::ErrorKind::NotFound => (),
            Err(e) => return Err(e),
        }
    }
    Ok(p)
}

#[derive(Serialize, Deserialize)]
pub struct Change {
    path: String,
    existed: bool,
}

fn replace(src: &Path, dest: &Path) -> io::Result<()> {
    let parent = dest.parent().unwrap();
    fs::create_dir_all(parent)?;
    let temp = NamedTempFile::new_in(parent)?;
    fs::copy(src, temp.path())?;
    temp.as_file().sync_all()?;
    temp.persist(dest)?;
    Ok(())
}
pub fn atomic_write(path: &Path, data: &[u8]) -> io::Result<()> {
    let mut tmp = NamedTempFile::new_in(path.parent().unwrap())?;
    tmp.write_all(data)?;
    tmp.as_file().sync_all()?;
    tmp.persist(path)?;
    Ok(())
}

pub fn prepare(
    root: &Path,
    work: &Path,
    old: &BTreeSet<String>,
    new: &BTreeSet<String>,
    mac: bool,
) -> io::Result<Vec<Change>> {
    let mut ops = vec![];
    for name in old.union(new) {
        let dest = safe_dest(root, name, mac)?;
        let exists = dest.try_exists()?;
        if exists && (!old.contains(name) || !dest.is_file()) {
            return Err(invalid("Conflicting local file"));
        }
        let source = work.join("new").join(name);
        if (exists && new.contains(name) && hash(&dest)? == hash(&source)?)
            || (!exists && !new.contains(name))
        {
            continue;
        }
        if exists {
            let backup = work.join("old").join(name);
            fs::create_dir_all(backup.parent().unwrap())?;
            if fs::hard_link(&dest, &backup).is_err() {
                fs::copy(&dest, &backup)?;
            }
        }
        ops.push(Change {
            path: name.to_string(),
            existed: exists,
        });
    }
    Ok(ops)
}

pub fn rollback(root: &Path, work: &Path, mac: bool) -> io::Result<()> {
    let journal = work.join("journal");
    if !journal.try_exists()? {
        return Ok(());
    }
    let ops: Vec<Change> = serde_json::from_reader(File::open(&journal)?)?;
    for op in ops.iter().rev() {
        let dest = safe_dest(root, &op.path, mac)?;
        if op.existed {
            let old = work.join("old").join(&op.path);
            if !dest.is_file() || hash(&dest)? != hash(&old)? {
                replace(&old, &dest)?;
            }
        } else if dest.try_exists()? {
            fs::remove_file(dest)?;
        }
    }
    fs::remove_file(journal)
}

pub fn apply(root: &Path, work: &Path, ops: &[Change], mac: bool) -> io::Result<()> {
    atomic_write(&work.join("journal"), &serde_json::to_vec(ops)?)?;
    for op in ops {
        let dest = safe_dest(root, &op.path, mac)?;
        let source = work.join("new").join(&op.path);
        if source.try_exists()? {
            replace(&source, &dest)?;
        } else {
            fs::remove_file(dest)?;
        }
    }
    fs::remove_file(work.join("journal"))
}
