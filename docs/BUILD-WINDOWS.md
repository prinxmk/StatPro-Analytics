# Build a genuine Windows Setup.exe using GitHub Actions

1. Create a GitHub repository.
2. Upload the complete contents of this project.
3. Commit `.github/workflows/windows-installer.yml`.
4. Push a version tag such as `v0.3.0`, or use GitHub Actions → Build Windows Installer → Run workflow.
5. GitHub starts a real Windows runner, installs Qt, compiles with MSVC, installs Inno Setup, and creates `StatPro-Analytics-Setup.exe`.
6. Open the completed workflow and download the `StatPro-Analytics-Setup` artifact.

This is a genuine Windows executable/installer built on a Windows VM, not a renamed archive.

The current workflow is x64. The x86 build should be a separate pipeline using a 32-bit Qt/MSVC kit after the x64 build is stable.
