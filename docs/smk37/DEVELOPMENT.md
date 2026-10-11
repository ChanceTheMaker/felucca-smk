# Felucca SMK development

Repository: <https://github.com/ChanceTheMaker/felucca-smk>

## Checkout and preview

```sh
git clone https://github.com/ChanceTheMaker/felucca-smk.git
cd felucca-smk
python tools/serve_smk37.py --port 8773
```

Open `http://localhost:8773/`, `/editor.html`, or `/simulator.html`.
Python 3 is the only server dependency. The prebuilt SMK WebAssembly runtime,
JavaScript, fonts, artwork and sample sources are in the repository. There are
no Git submodules or sibling-checkout dependencies for the website. The server
defaults to no staged hardware package; browser Studio and Simulator still work.
`tools/make_smk_site.py build/smk-site` stages the public website with relative
paths for GitHub Pages. `.github/workflows/smk-pages.yml` deploys `main` in the
SMK repository. Private build files are excluded from staging.

The migration preserves the SMK branch history in `main` and
`feat/smk-studio-simulator`. The upstream source reference is Felucca 1.5.1,
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`. The original FM-1 repository remains
separate. Prior PR links in historical documents still refer to that repository.

## Rebuild the browser runtime

Use Python 3.10 or newer, Pillow 11.0.0 with **libraqm**, Zig 0.13.0 and Node.js
22. On Windows, run the build commands below inside WSL/Linux: the Windows
Pillow installation may lack libraqm, which the display font generator requires.
This requirement does not apply to previewing the checked-in runtime.

```sh
python -m venv build/browser-env
build/browser-env/bin/python -m pip install -r tools/smk-browser-requirements.txt
build/browser-env/bin/python tools/build_smk37_browser.py
node web/audio/test_smk_engine.mjs
node web/audio/test_smk_worklet.mjs
node web/test_keyboard_glide.mjs
node web/test_web.mjs
```

If a minimal Linux/WSL installation has pip but no `venv`/`ensurepip`, install
only into this checkout instead:

```sh
python3 -m pip install --target build/browser-tools -r tools/smk-browser-requirements.txt
PYTHONPATH="$PWD/build/browser-tools" python3 tools/build_smk37_browser.py
```

The builder locates Zig from `--zig`, the `ZIG` environment variable, the
installed `ziglang` package, `build/wasm-tools/ziglang`, or `PATH`, in that order.
It generates its own headers from checked-in sources and assets. Do not use
`--skip-generate` for a first build. It writes `web/audio/smk-engine.wasm` and
its matching SHA-256 manifest `web/audio/smk-build.json`; commit both together.
Browser sessions are keyed by runtime revision, so a changed binary creates a
distinct session store. The browser build does not open or write a device.

## Hardware firmware dependencies

SMK hardware builds use `tools/build_smk37.py` and the SMK loader/package tools,
not the FM-1 `build.sh` release procedure. Read [Test21](TEST21.md) and inspect
`python tools/build_smk37.py --help` before preparing a hardware candidate.

- JieLi pi32v2 Linux toolchain: bootstrap with `tools/get_toolchain.sh`; set
  `JIELI_TOOLCHAIN` to the resulting toolchain directory.
- Linux x86-64, or Docker Desktop for the Linux toolchain on Windows/macOS.
  The build uses `debian:bookworm-slim` unless `JIELI_DOCKER_IMAGE` overrides it.
- Python/Pillow and checked-in fonts/sample assets for generated headers.
  Pillow must include libraqm; the same WSL/Linux environment above can generate
  hardware display fonts without changing the upstream rasterizer.
- Vendor SDK packaging inputs, where required, come from
  `AC79NN_SDK_V1.2.1_2023-12-13`; see [BUILDING.md](../../BUILDING.md) for
  `AC79_SDK` setup. These are not silently substituted from another checkout.
- MIDI tooling only: `python -m pip install -r tools/smk37-requirements.txt`.
- Device-specific stock backups, calibration and stock package inputs remain
  private. Features that require verified paired backups still require them.

Existing toolchains may be reused through explicit paths. Generated build
directories, device captures and recovery material are not source dependencies
to upload to GitHub. Keep them outside version control and backed up locally.
The original local worktree and its private Test21 artifacts are retained during
migration, so existing installer servers and recovery evidence remain available.

## Release boundary

Product version: **0.9.21 Beta**. Existing private Test19–Test21 package identities
remain unchanged. The source migration does not publish a website, create a
new installable firmware package, or flash a device. Known hardware follow-ups
remain in [Test21](TEST21.md) and [WORK_QUEUE.md](../WORK_QUEUE.md).
License and attribution files travel with the source and generated runtime.
