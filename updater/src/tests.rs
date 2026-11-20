use super::*;
use serde_json::json;
use std::collections::BTreeSet;
fn asset(v: &str, suffix: &str) -> serde_json::Value {
    let name = format!("SonicCompilationUltra-{v}-{suffix}");
    json!({"tag_name":format!("runtime-preview-{v}"),"draft":false,"assets":[{"name":name,"size":123,"digest":format!("sha256:{}","a".repeat(64)),"state":"uploaded","browser_download_url":format!("https://github.com/{}/releases/download/runtime-preview-{v}/{name}",feed::REPO)}]})
}
#[test]
fn checks_numeric_preview_matching_platform_and_integrity() {
    let suffix = "windows-x64.zip";
    let mut draft = asset("4.0.0", suffix);
    draft["draft"] = true.into();
    let mut nohash = asset("9.0.0", suffix);
    nohash["assets"][0]["digest"] = serde_json::Value::Null;
    let mut badurl = asset("8.0.0", suffix);
    badurl["assets"][0]["browser_download_url"] = "https://example.org/wrong".into();
    let data = serde_json::to_vec(&vec![
        asset("0.9.10", suffix),
        asset("0.16.1", suffix),
        draft,
        nohash,
        badurl,
        asset("3.0.0", "linux-x64.tar.gz"),
    ])
    .unwrap();
    assert_eq!(
        feed::select(&data, "0.15.0", suffix)
            .unwrap()
            .unwrap()
            .version,
        "0.16.1"
    );
    assert!(feed::select(&data, "0.16.1", suffix).unwrap().is_none());
    assert!(feed::select(&data, "1.0.0", suffix).unwrap().is_none());
    assert!(feed::version("1.03.2").is_none());
    assert!(feed::version("1.0.2-rogue").is_none());
}
#[test]
fn transaction_preserves_data_and_recovers_partial_install() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("game");
    let work = tmp.path().join("work");
    for d in [
        root.join("UserData/saves"),
        root.join("ROMs"),
        work.join("new"),
    ] {
        fs::create_dir_all(d).unwrap();
    }
    for n in ["UserData/saves/a.state", "ROMs/a.gen", "user.txt"] {
        fs::write(root.join(n), b"personal").unwrap();
    }
    fs::write(root.join("SonicCompilationUltra.exe"), b"old").unwrap();
    fs::write(root.join("CHANGELOG.md"), b"obsolete").unwrap();
    let old = BTreeSet::from(["SonicCompilationUltra.exe".into(), "CHANGELOG.md".into()]);
    let new = BTreeSet::from(["SonicCompilationUltra.exe".into(), "README.md".into()]);
    for n in &new {
        fs::write(work.join("new").join(n), b"new").unwrap();
    }
    let ops = package::prepare(&root, &work, &old, &new, false).unwrap();
    package::apply(&root, &work, &ops, false).unwrap();
    assert!(!root.join("CHANGELOG.md").exists());
    assert_eq!(
        fs::read(root.join("SonicCompilationUltra.exe")).unwrap(),
        b"new"
    );
    package::atomic_write(&work.join("journal"), &serde_json::to_vec(&ops).unwrap()).unwrap();
    package::rollback(&root, &work, false).unwrap();
    assert_eq!(
        fs::read(root.join("SonicCompilationUltra.exe")).unwrap(),
        b"old"
    );
    assert!(!root.join("README.md").exists());
    assert!(root.join("CHANGELOG.md").exists());
    for n in ["UserData/saves/a.state", "ROMs/a.gen", "user.txt"] {
        assert_eq!(fs::read(root.join(n)).unwrap(), b"personal");
    }
}
#[test]
fn refuses_paths_outside_program() {
    let p = Path::new(".");
    for s in [
        "../x",
        "ROMs/test.gen",
        "assets/../../UserData",
        "assets/C:/outside",
        "assets/foo\\bar",
        "assets/CON",
        "/absolute",
    ] {
        assert!(package::safe_dest(p, s, false).is_err(), "{s}");
    }
    assert!(package::safe_dest(p, "Contents/Resources/assets/a", true).is_ok());
    assert!(package::safe_dest(p, "Contents/Resources/UserData/x", true).is_err());
}

#[test]
fn release_archives_enforce_identity_content_and_permissions() {
    use std::io::Cursor;
    use std::io::Write;
    for target in [
        "windows-x64.zip",
        "linux-x64.tar.gz",
        "macos-x64.tar.gz",
        "macos-arm64.tar.gz",
    ] {
        let t = tempfile::tempdir().unwrap();
        let (platform, rest) = target.split_once('-').unwrap();
        let arch = rest.split('.').next().unwrap();
        let launcher = match platform {
            "windows" => "SonicCompilationUltra.exe",
            "linux" => "SonicCompilationUltra.sh",
            _ => "Sonic Compilation Ultra.app/Contents/MacOS/SonicCompilationUltra",
        };
        let path = t.path().join("package");
        let root = format!("SonicCompilationUltra-1.0.0-{platform}-{arch}");
        for corrupted in [false, true] {
            let body = b"test application";
            let hash = format!(
                "{:x}",
                Sha256::digest(if corrupted {
                    b"altered".as_slice()
                } else {
                    body.as_slice()
                })
            );
            let manifest=serde_json::to_vec(&json!({"version":"1.0.0","architecture":arch,"platform":platform,"game_data_included":false,
                "files":[{"path":launcher,"bytes":body.len(),"sha256":hash}]})).unwrap();
            let members = [
                (launcher, body.as_slice(), 0o755),
                ("MANIFEST.json", manifest.as_slice(), 0o644),
            ];
            let file = File::create(&path).unwrap();
            if platform == "windows" {
                let mut z = zip::ZipWriter::new(file);
                for (n, b, mode) in members {
                    z.start_file(
                        format!("{root}/{n}"),
                        zip::write::FileOptions::default().unix_permissions(mode),
                    )
                    .unwrap();
                    z.write_all(b).unwrap();
                }
                z.finish().unwrap();
            } else {
                let enc = flate2::write::GzEncoder::new(file, flate2::Compression::fast());
                let mut tar = tar::Builder::new(enc);
                for (n, b, mode) in members {
                    let mut head = tar::Header::new_gnu();
                    head.set_size(b.len() as u64);
                    head.set_mode(mode);
                    head.set_cksum();
                    tar.append_data(&mut head, format!("{root}/{n}"), Cursor::new(b))
                        .unwrap();
                }
                tar.into_inner().unwrap().finish().unwrap();
            }
            let out = tempfile::tempdir().unwrap();
            let u = Update {
                version: "1.0.0".into(),
                name: "package".into(),
                url: String::new(),
                size: 0,
                sha: String::new(),
            };
            let res = package::stage(&path, out.path(), &u, target);
            if corrupted {
                assert!(res.is_err());
                continue;
            }
            let local = launcher
                .strip_prefix("Sonic Compilation Ultra.app/")
                .unwrap_or(launcher);
            assert!(res.unwrap().contains(local));
            let dest = out.path().join(local);
            assert_eq!(fs::read(&dest).unwrap(), body);
            #[cfg(unix)]
            {
                use std::os::unix::fs::PermissionsExt;
                assert_ne!(fs::metadata(dest).unwrap().permissions().mode() & 0o111, 0);
            }
        }
    }
}
#[test]
fn matches_real_release_package_when_supplied() {
    let Some(path) = std::env::var_os("SCU_TEST_RELEASE_PACKAGE") else {
        return;
    };
    let path = Path::new(&path);
    let target = feed::target();
    let name = path.file_name().unwrap().to_str().unwrap();
    let ver = name
        .strip_prefix("SonicCompilationUltra-")
        .unwrap()
        .strip_suffix(target)
        .unwrap()
        .trim_end_matches('-');
    let u = Update {
        version: ver.into(),
        name: name.into(),
        url: String::new(),
        size: 0,
        sha: String::new(),
    };
    let tmp = tempfile::tempdir().unwrap();
    let files = package::stage(path, tmp.path(), &u, target).unwrap();
    assert!(files.len() > 10);
    let prior = package::prior(tmp.path(), cfg!(target_os = "macos")).unwrap();
    assert_eq!(
        files, prior,
        "installed ownership inventory must match the released package"
    );
}
