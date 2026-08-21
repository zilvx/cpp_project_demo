#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
跨平台打包脚本 —— 将本项目打包为 macOS / Linux / Windows 可分发程序。

用法示例：
    python3 packaging/build_package.py                        # 全量构建并打包当前平台
    python3 packaging/build_package.py -t qt                  # 仅打包 Qt 桌面应用
    python3 packaging/build_package.py --data-source http     # 以 HTTP JSON 数据源构建
    python3 packaging/build_package.py --skip-build           # 只打包已有构建产物

各平台产物：
    macOS   -> build/dist/macOS/QtDemo-<版本>-macos.dmg   （含 Qt 框架，可独立运行）
    Windows -> build/dist/Windows/QtDemo-<版本>-windows-x86_64.zip
    Linux   -> build/dist/Linux/QtDemo-<版本>-linux-x86_64.tar.gz

注意：
    * Windows 需在 MSVC 开发者环境（vcvars64）中运行；Qt 路径用 --qt-dir 或 QT_DIR 指定。
    * Linux 依赖 patchelf 改写 rpath（缺失时仅告警，产物可能不可移植）。
    * 脚本只在当前运行的操作系统上打包（CMake/Conan 不支持跨系统交叉产物）。
"""

import argparse
import glob
import os
import platform
import re
import shutil
import subprocess
import sys

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_ROOT, "build")
SYSTEM = platform.system()


# ============================================================
#  基础工具
# ============================================================

def run(cmd, cwd=None, check=True):
    """执行命令并打印输出；check=True 时失败即抛异常。"""
    print("+ " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=cwd, text=True)
    if check and proc.returncode != 0:
        raise RuntimeError("命令失败: " + " ".join(cmd))
    return proc.returncode


def run_out(cmd, cwd=None):
    """执行命令并返回 (returncode, stdout)。"""
    proc = subprocess.run(cmd, cwd=cwd, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return proc.returncode, proc.stdout


def qrun(cmd, cwd=None):
    """静默执行命令（install_name_tool 等低层工具，忽略输出）。"""
    subprocess.run(cmd, cwd=cwd,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def parse_version():
    """从 CMakeLists.txt 解析 project(... VERSION x.y.z)。"""
    path = os.path.join(PROJECT_ROOT, "CMakeLists.txt")
    with open(path, encoding="utf-8") as f:
        text = f.read()
    m = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+\.\d+)", text)
    return m.group(1) if m else "1.0.0"


def detect_qt_dir():
    """探测 Qt 安装根目录。"""
    env = os.environ.get("QT_DIR")
    if env and os.path.isdir(env):
        return env
    if SYSTEM == "Darwin":
        for cand in ("/opt/homebrew/opt/qt", "/usr/local/opt/qt"):
            if os.path.isdir(cand):
                return cand
    elif SYSTEM == "Windows":
        for pat in (r"C:\Qt\6.*", r"D:\Qt\6.*"):
            for cand in glob.glob(pat):
                return cand
    elif SYSTEM == "Linux":
        for cand in ("/usr", "/usr/lib/qt6", "/opt/qt6"):
            if os.path.isdir(cand):
                return cand
    return ""


def find_conan_toolchain():
    """定位 Conan 生成的 conan_toolchain.cmake（AGENTS.md: 嵌套在 .../Release/generators/）。"""
    matches = glob.glob(os.path.join(BUILD_DIR, "**", "generators", "conan_toolchain.cmake"),
                        recursive=True)
    if not matches:
        return None
    return sorted(matches)[-1]


def find_file(root, pattern):
    """在 root 下递归查找文件名（返回第一个）。"""
    for m in glob.glob(os.path.join(root, "**", pattern), recursive=True):
        if os.path.isfile(m):
            return m
    return None


def qt_lib_dir(qt_dir):
    if SYSTEM == "Darwin":
        return os.path.join(qt_dir, "lib")
    if SYSTEM == "Windows":
        for d in glob.glob(os.path.join(qt_dir, "lib", "cmake")):
            return os.path.dirname(d)
        return os.path.join(qt_dir, "lib")
    # Linux: 系统 Qt 时插件在 /usr/lib/<arch>/qt6，这里允许 qt_dir 直接指向 lib 或根
    for cand in (os.path.join(qt_dir, "lib"), qt_dir):
        if os.path.isdir(cand):
            return cand
    return qt_dir


# ============================================================
#  目标与构建
# ============================================================

TARGET_GROUPS = {
    "qt": ["qt_table_app", "qt_table_app_fork", "fetch_table_child"],
    "server": ["table_data_server", "table_data_server_prefork"],
    "console": ["main"],
}

APP_BUNDLES = ["qt_table_app", "qt_table_app_fork"]
CLI_EXES = ["table_data_server", "table_data_server_prefork", "main", "fetch_table_child"]


def selected_targets(args):
    """解析 -t 参数；"all" 展开为全部组。"""
    ts = set(args.target.split(","))
    if "all" in ts:
        ts = set(TARGET_GROUPS.keys())
    return ts


def selected_artifacts(targets):
    """按目标选择要打包的产物名（Windows/Linux 下 qt 也是 .exe）。"""
    names = []
    for t in sorted(targets):
        names += TARGET_GROUPS.get(t, [])
    return names


def selected_cli_exes(targets):
    """按目标选择要打包的 CLI 可执行文件（macOS 下的 .app 单独处理）。"""
    exes = []
    if "qt" in targets:
        exes.append("fetch_table_child")
    if "server" in targets:
        exes += ["table_data_server", "table_data_server_prefork"]
    if "console" in targets:
        exes.append("main")
    return exes


def step_build(args):
    """conan install -> cmake configure -> cmake build。"""
    if args.skip_build:
        print("[构建] 跳过构建（--skip-build），使用现有产物")
        return

    btype = args.build_type
    data_flag = {
        "sample": [],
        "http": ["-DUSE_HTTP_DATA=ON"],
        "proto": ["-DUSE_PROTO_DATA=ON"],
    }[args.data_source]

    run(["conan", "install", ".", "--output-folder=.", "--build=missing",
         "-s", f"build_type={btype}"], cwd=PROJECT_ROOT)

    toolchain = find_conan_toolchain()
    if not toolchain:
        raise RuntimeError("未找到 conan_toolchain.cmake，请检查 conan install 是否成功")
    print(f"[构建] 使用工具链: {toolchain}")

    cfg = ["cmake", "-S", ".", "-B", "build",
           f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
           f"-DCMAKE_BUILD_TYPE={btype}", *data_flag]
    if SYSTEM == "Darwin" and os.path.isdir("/opt/homebrew/opt/qt"):
        cfg.append("-DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt")
    run(cfg, cwd=PROJECT_ROOT)

    targets = []
    for name in selected_targets(args):
        targets.extend(TARGET_GROUPS[name])
    run(["cmake", "--build", "build", "--config", btype, "--target"] + targets + ["-j"],
        cwd=PROJECT_ROOT)


def build_artifact(name):
    """返回构建产物路径（.app 为目录，其余为可执行文件）。"""
    if SYSTEM == "Darwin" and name in APP_BUNDLES:
        return os.path.join(BUILD_DIR, f"{name}.app")
    ext = ".exe" if SYSTEM == "Windows" else ""
    return os.path.join(BUILD_DIR, name + ext)


def copy_common(stage_dir):
    """复制 data/ 与 README 等通用资源。"""
    os.makedirs(os.path.join(stage_dir, "data"), exist_ok=True)
    shutil.copy2(os.path.join(PROJECT_ROOT, "data", "arbitrary_wave.csv"),
                 os.path.join(stage_dir, "data", "arbitrary_wave.csv"))
    if os.path.exists(os.path.join(PROJECT_ROOT, "README.md")):
        shutil.copy2(os.path.join(PROJECT_ROOT, "README.md"),
                     os.path.join(stage_dir, "README.md"))


def stage_cli_binaries(stage_dir, names):
    """把 CLI 可执行文件复制到 staging/bin/。"""
    bin_dir = os.path.join(stage_dir, "bin")
    os.makedirs(bin_dir, exist_ok=True)
    for name in names:
        src = build_artifact(name)
        if not os.path.exists(src):
            print(f"[警告] 缺少产物 {src}，跳过")
            continue
        shutil.copy2(src, os.path.join(bin_dir, os.path.basename(src)))
    return bin_dir


# ============================================================
#  macOS 打包
# ============================================================

MAC_FRAMEWORK_RE = re.compile(r"(?:@rpath/|/.*/)(Qt[A-Za-z0-9_]+)\.framework/Versions/A/\1")


def mac_tool(qt_dir, name):
    """查找 macdeployqt / otool / install_name_tool / codesign。"""
    for cand in (os.path.join(qt_dir, "bin", name),
                 os.path.join(qt_dir, "libexec", name)):
        if os.path.isfile(cand):
            return cand
    return shutil.which(name)


def mac_deps(obj):
    """otool -L 解析直接依赖列表。"""
    _, out = run_out(["otool", "-L", obj])
    deps = []
    for line in out.splitlines()[1:]:
        line = line.strip()
        if not line:
            continue
        deps.append(line.split()[0])
    return deps


def mac_rewrite(obj, mapping):
    """用 install_name_tool -change 批量改写依赖引用。"""
    for old, new in mapping.items():
        if old != new:
            qrun(["install_name_tool", "-change", old, new, obj])


def mac_rpaths(exe):
    """解析可执行文件的 LC_RPATH 列表（不能用子串匹配，LC_LOAD_DYLIB 名也含 @executable_path）。"""
    _, out = run_out(["otool", "-l", exe])
    return set(re.findall(r"path (.*?) \(offset", out))


def mac_add_rpath(exe, rpath):
    """幂等地为可执行文件添加 rpath。"""
    if rpath in mac_rpaths(exe):
        return
    qrun(["install_name_tool", "-add_rpath", rpath, exe])


def mac_del_rpath(exe, rpath):
    """删除机器相关的死 rpath。"""
    qrun(["install_name_tool", "-delete_rpath", rpath, exe])


def mac_del_machine_rpaths(exe):
    """删除所有指向本机绝对路径的 rpath（打包后不可移植）。"""
    for rp in mac_rpaths(exe):
        if rp.startswith("/"):
            mac_del_rpath(exe, rp)


def mac_framework_target(name, prefix="@executable_path/../Frameworks"):
    """部署后的框架引用目标路径。

    .app bundle:  exe 在 Contents/MacOS，Frameworks 在 Contents/Frameworks -> ../Frameworks
    CLI 工具:      exe 与 Frameworks 同在 bin/ -> @executable_path/Frameworks
    """
    return f"{prefix}/{name}.framework/Versions/A/{name}"


def mac_copy_framework(qt_dir, frameworks_dir, name):
    """把 Qt 框架复制到 bundle 的 Frameworks/ 目录。

    必须 symlinks=True 保留框架顶部 symlink（QtDBus -> Versions/Current/QtDBus），
    否则会复制出重复二进制导致 codesign 报 "bundle format is ambiguous"。
    """
    src = os.path.join(qt_lib_dir(qt_dir), f"{name}.framework")
    dst = os.path.join(frameworks_dir, f"{name}.framework")
    if not os.path.isdir(src):
        raise RuntimeError(f"缺少 Qt 框架: {src}")
    if not os.path.isdir(dst):
        shutil.copytree(src, dst, symlinks=True)
    return dst


def mac_bundle_qt_for_cli(exes, qt_dir, frameworks_dir):
    """为 CLI 工具手动部署 Qt 框架（macdeployqt 不支持裸可执行文件）。

    将 QtCore/QtNetwork 等框架复制进 <bin>/Frameworks/，并把
    @rpath/绝对路径引用改写为 @executable_path/Frameworks/...，
    最后为每个可执行文件添加该 rpath。
    """
    os.makedirs(frameworks_dir, exist_ok=True)
    qt_lib = qt_lib_dir(qt_dir)
    prefix = "@executable_path/Frameworks"

    # 1) 解析需要的 Qt 框架（闭包）
    needed = {}
    queue = list(exes)
    while queue:
        obj = queue.pop()
        for dep in mac_deps(obj):
            m = MAC_FRAMEWORK_RE.search(dep)
            if m:
                name = m.group(1)
                if name not in needed:
                    needed[name] = mac_framework_target(name, prefix)
                    queue.append(os.path.join(qt_lib, f"{name}.framework",
                                              "Versions", "A", name))
    if not needed:
        print("[macOS] CLI 工具无 Qt 依赖，跳过框架部署")
        return

    # 2) 复制框架
    for name in needed:
        mac_copy_framework(qt_dir, frameworks_dir, name)

    # 3) 改写所有对象的引用（可执行文件 + 框架二进制）
    objects = list(exes)
    for name in needed:
        objects.append(os.path.join(frameworks_dir, f"{name}.framework",
                                    "Versions", "A", name))
    for obj in objects:
        mapping = {}
        for dep in mac_deps(obj):
            m = MAC_FRAMEWORK_RE.search(dep)
            if m:
                mapping[dep] = needed[m.group(1)]
            elif dep.startswith(qt_lib + "/"):
                m2 = MAC_FRAMEWORK_RE.search(dep)
                if m2:
                    mapping[dep] = needed[m2.group(1)]
        mac_rewrite(obj, mapping)

    # 4) 设置框架 install name，添加 rpath，重签
    for name, target in needed.items():
        fb = os.path.join(frameworks_dir, f"{name}.framework", "Versions", "A", name)
        qrun(["install_name_tool", "-id", target, fb])
        # -id 会使 Homebrew 的 ad-hoc 签名失效，Apple Silicon 上必须重签，否则进程被 AMFI 杀掉
        qrun(["codesign", "--force", "--sign", "-",
              os.path.join(frameworks_dir, f"{name}.framework")])
    for exe in exes:
        mac_add_rpath(exe, prefix)
        mac_del_machine_rpaths(exe)
        mac_sign(exe)


def mac_ensure_agl_framework(bundle_frameworks_dir):
    """在 bundle 内构造 codesign 合法的 AGL.framework（Qt 仍链接 AGL，macOS 26 起系统移除）。

    macdeployqt 无法解析 stub AGL（rpath 指向源码目录），且原始 stub 缺少
    Resources/Info.plist 导致 codesign 拒绝 —— 这里重建一个合法最小框架。
    """
    agl = os.path.join(bundle_frameworks_dir, "AGL.framework")
    if os.path.isdir(agl):
        return
    src = os.path.join(PROJECT_ROOT, "stub_frameworks", "AGL.framework",
                       "Versions", "A", "AGL")
    if not os.path.isfile(src):
        raise RuntimeError("缺少 stub_frameworks/AGL.framework")
    os.makedirs(os.path.join(agl, "Versions", "A", "Resources"), exist_ok=True)
    shutil.copy2(src, os.path.join(agl, "Versions", "A", "AGL"))
    plist = os.path.join(agl, "Versions", "A", "Resources", "Info.plist")
    with open(plist, "w", encoding="utf-8") as f:
        f.write('''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
\t<key>CFBundleIdentifier</key>\t<string>com.apple.AGL</string>
\t<key>CFBundleName</key>\t<string>AGL</string>
\t<key>CFBundlePackageType</key>\t<string>FMWK</string>
\t<key>CFBundleVersion</key>\t<string>1.0</string>
\t<key>CFBundleShortVersionString</key>\t<string>1.0</string>
</dict>
</plist>
''')
    os.symlink("A", os.path.join(agl, "Versions", "Current"))
    os.symlink("Versions/Current/AGL", os.path.join(agl, "AGL"))
    os.symlink("Versions/Current/Resources", os.path.join(agl, "Resources"))


def mac_sign(obj):
    """ad-hoc 签名（Apple Silicon 上修改过二进制后必须重签）。"""
    qrun(["codesign", "--force", "--sign", "-", obj])


def mac_core_frameworks(exe, fw_dir):
    """从主可执行文件出发做 BFS，求真正需要的 Qt 框架闭包（不含可选插件）。"""
    needed = set()
    queue = [exe]
    while queue:
        obj = queue.pop()
        for dep in mac_deps(obj):
            m = MAC_FRAMEWORK_RE.search(dep)
            if m and m.group(1) not in needed:
                needed.add(m.group(1))
                fw = os.path.join(fw_dir, f"{m.group(1)}.framework",
                                  "Versions", "A", m.group(1))
                if os.path.isfile(fw):
                    queue.append(fw)
    return needed


def mac_repair_bundle(bundle, qt_dir):
    """macdeployqt 之后的修复（该 Homebrew Qt 下 macdeployqt 有缺陷）：

    1) 求主可执行文件的 Qt 框架闭包，清理可选的无关框架/插件
    2) 为可执行文件添加 @executable_path/../Frameworks rpath
    3) 复制缺失的传递 Qt 框架（如 QtGui -> QtDBus）
    4) 删除机器相关死 rpath
    5) 重建 AGL.framework、整体 ad-hoc 重签
    """
    macos_dir = os.path.join(bundle, "Contents", "MacOS")
    fw_dir = os.path.join(bundle, "Contents", "Frameworks")
    exe = os.path.join(macos_dir, os.listdir(macos_dir)[0])
    qt_lib = qt_lib_dir(qt_dir)

    # 1) 核心框架闭包 + 清理无关 Qt 框架（macdeployqt 会把可选插件依赖全拷进来）
    core = mac_core_frameworks(exe, fw_dir)
    for name in list(os.listdir(fw_dir)):
        if name.endswith(".framework") and name[:-10] not in core:
            print(f"[macOS] 清理可选框架: {name}")
            shutil.rmtree(os.path.join(fw_dir, name), ignore_errors=True)

    # 2) rpath
    mac_add_rpath(exe, "@executable_path/../Frameworks")

    # 3) 复制缺失的核心框架并设置 -id
    for name in core:
        if not os.path.isdir(os.path.join(fw_dir, f"{name}.framework")):
            mac_copy_framework(qt_dir, fw_dir, name)
            fb = os.path.join(fw_dir, f"{name}.framework", "Versions", "A", name)
            qrun(["install_name_tool", "-id", mac_framework_target(name), fb])

    # 4) 清理引用缺失框架的可选插件（否则 dyld 报错）
    for plug in glob.glob(os.path.join(bundle, "Contents", "PlugIns", "**", "*.dylib"),
                          recursive=True):
        for dep in mac_deps(plug):
            m = MAC_FRAMEWORK_RE.search(dep)
            if m and m.group(1) not in core:
                print(f"[macOS] 移除依赖缺失框架的插件: {os.path.basename(plug)}")
                os.remove(plug)
                break

    # 5) AGL 与死 rpath
    mac_ensure_agl_framework(fw_dir)
    mac_del_machine_rpaths(exe)

    # 6) 重签
    run(["codesign", "--force", "--deep", "--sign", "-", bundle], check=False)


def mac_verify_bundle(bundle):
    """校验 bundle 内不存在引用缺失框架的 dylib。"""
    fw_dir = os.path.join(bundle, "Contents", "Frameworks")
    bad = []
    for obj in glob.glob(os.path.join(bundle, "Contents", "**", "*"), recursive=True):
        if not os.path.isfile(obj):
            continue
        for dep in mac_deps(obj):
            m = MAC_FRAMEWORK_RE.search(dep)
            if m and not os.path.isdir(os.path.join(fw_dir, f"{m.group(1)}.framework")):
                bad.append((os.path.basename(obj), m.group(1)))
    if bad:
        raise RuntimeError(f"bundle 校验失败，存在未打包框架: {bad}")


def package_macos(args, version):
    print("\n===== macOS 打包 =====")
    qt_dir = args.qt_dir
    targets = selected_targets(args)
    macdeployqt = mac_tool(qt_dir, "macdeployqt") or mac_tool(qt_dir, "macdeployqt6")
    if not macdeployqt:
        raise RuntimeError("未找到 macdeployqt（Qt 安装不完整）")

    stage = os.path.join(args.out, "macOS", f"QtDemo-{version}-macos")
    if os.path.isdir(stage):
        shutil.rmtree(stage)
    os.makedirs(stage)

    # --- 部署 .app bundle（仅 qt 目标） ---
    if "qt" in targets:
        for name in APP_BUNDLES:
            src = build_artifact(name)
            dst = os.path.join(stage, os.path.basename(src))
            shutil.copytree(src, dst, symlinks=True)
            print(f"[macOS] macdeployqt {name}.app ...")
            run_out([macdeployqt, dst])
            mac_repair_bundle(dst, qt_dir)
            mac_verify_bundle(dst)
            if name == "qt_table_app_fork":
                # 子进程程序放入 bundle，供 ForkedHttpDataProvider 定位
                child = build_artifact("fetch_table_child")
                if os.path.exists(child):
                    child_dst = os.path.join(dst, "Contents", "MacOS", "fetch_table_child")
                    shutil.copy2(child, child_dst)
                    mac_add_rpath(child_dst, "@executable_path/../Frameworks")
                    mac_sign(child_dst)
                qrun(["codesign", "--force", "--deep", "--sign", "-", dst])

    # --- CLI 工具：独立 Frameworks 部署 ---
    cli = selected_cli_exes(targets)
    if cli:
        bin_dir = stage_cli_binaries(stage, cli)
        deployed = [os.path.join(bin_dir, f) for f in os.listdir(bin_dir)]
        mac_bundle_qt_for_cli(deployed, qt_dir, os.path.join(bin_dir, "Frameworks"))

    copy_common(stage)

    # --- 生成 .dmg ---
    dmg = os.path.join(args.out, "macOS", f"QtDemo-{version}-macos.dmg")
    volname = f"QtDemo-{version}"
    run(["hdiutil", "create", "-volname", volname, "-srcfolder", stage,
         "-ov", "-format", "UDZO", dmg])
    print(f"[macOS] 产物: {dmg}")
    return dmg


# ============================================================
#  Windows 打包
# ============================================================

def package_windows(args, version):
    print("\n===== Windows 打包 =====")
    qt_dir = args.qt_dir
    windeployqt = find_file(qt_dir, "windeployqt.exe")
    if not windeployqt:
        raise RuntimeError("未找到 windeployqt.exe（--qt-dir 指向 Qt 安装目录）")

    stage = os.path.join(args.out, "Windows", f"QtDemo-{version}-windows")
    if os.path.isdir(stage):
        shutil.rmtree(stage)
    os.makedirs(stage)

    cli = selected_artifacts(selected_targets(args))
    for name in cli:
        src = build_artifact(name)
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(stage, os.path.basename(src)))

    # windeployqt 收集 Qt DLL 与插件（仅 Qt 相关可执行文件，自动合并）
    for name in cli:
        exe = os.path.join(stage, name + ".exe")
        if os.path.exists(exe):
            run([windeployqt, "--release", "--no-translations", exe], check=False)

    copy_common(stage)
    zip_path = shutil.make_archive(
        os.path.join(args.out, "Windows", f"QtDemo-{version}-windows-x86_64"),
        "zip", stage)
    print(f"[Windows] 产物: {zip_path}")
    return zip_path


# ============================================================
#  Linux 打包
# ============================================================

SYSTEM_LIB_DIRS = ("/usr/lib", "/usr/lib64", "/lib", "/lib64", "/lib/x86_64-linux-gnu",
                   "/usr/lib/x86_64-linux-gnu", "/System")


def linux_is_system_lib(path):
    """判断是否为系统库（无需打包）。"""
    if path.startswith(SYSTEM_LIB_DIRS):
        return True
    if path.startswith("linux-vdso") or path.startswith("ld-"):
        return True
    if "/ld-linux" in path:
        return True
    return False


def linux_ldd_libs(binary):
    """ldd 解析动态库路径。"""
    _, out = run_out(["ldd", binary])
    libs = []
    for line in out.splitlines():
        line = line.strip()
        if "=>" in line:
            path = line.split("=>")[1].strip().split()[0]
            if os.path.isabs(path) and not linux_is_system_lib(path):
                libs.append(path)
        elif line and line.startswith("/"):
            libs.append(line)
    return libs


def linux_find_qt_plugins(qt_dir):
    """定位 Qt 插件目录（platforms/ 等）。"""
    for cand in (os.path.join(qt_dir, "plugins"),
                 os.path.join(qt_dir, "lib", "plugins"),
                 "/usr/lib/x86_64-linux-gnu/qt6/plugins",
                 "/usr/lib/qt6/plugins"):
        if os.path.isdir(os.path.join(cand, "platforms")):
            return cand
    return None


def package_linux(args, version):
    print("\n===== Linux 打包 =====")
    qt_dir = args.qt_dir
    patchelf = shutil.which("patchelf")
    if not patchelf:
        print("[警告] 未找到 patchelf，无法改写 rpath，产物可能不可移植")

    stage = os.path.join(args.out, "Linux", f"QtDemo-{version}-linux")
    if os.path.isdir(stage):
        shutil.rmtree(stage)
    bin_dir = os.path.join(stage, "bin")
    lib_dir = os.path.join(stage, "lib")
    plug_dir = os.path.join(stage, "plugins")
    os.makedirs(bin_dir); os.makedirs(lib_dir); os.makedirs(plug_dir)

    # --- 复制可执行文件与 Qt 插件 ---
    cli = selected_artifacts(selected_targets(args))
    for name in cli:
        src = build_artifact(name)
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(bin_dir, os.path.basename(src)))

    plugins = linux_find_qt_plugins(qt_dir)
    if plugins:
        for cat in ("platforms", "styles", "imageformats", "iconengines",
                    "xcbglintegrations", "platformthemes"):
            src_cat = os.path.join(plugins, cat)
            if os.path.isdir(src_cat):
                shutil.copytree(src_cat, os.path.join(plug_dir, cat))
    else:
        print("[警告] 未找到 Qt 插件目录，GUI 应用可能无法启动")

    # --- ldd 收集所有非系统动态库 ---
    objs = [os.path.join(bin_dir, f) for f in os.listdir(bin_dir)]
    objs += [os.path.join(plug_dir, f) for f in os.listdir(plug_dir) if f.endswith(".so")]
    collected = set()
    for obj in objs:
        if not os.path.isfile(obj):
            continue
        for lib in linux_ldd_libs(obj):
            collected.add(lib)
    for lib in sorted(collected):
        shutil.copy2(lib, os.path.join(lib_dir, os.path.basename(lib)))

    # --- rpath 改写 ---
    if patchelf:
        for obj in objs:
            if os.path.isfile(obj):
                run([patchelf, "--set-rpath", "$ORIGIN/../lib", obj])
        for lib in os.listdir(lib_dir):
            run([patchelf, "--set-rpath", "$ORIGIN", os.path.join(lib_dir, lib)],
                check=False)

    # --- qt.conf（相对 bin/ 定位插件与库） ---
    with open(os.path.join(bin_dir, "qt.conf"), "w", encoding="utf-8") as f:
        f.write("[Paths]\nPrefix = ../\nPlugins = ../plugins\nLibraries = ../lib\n")

    copy_common(stage)
    tar_path = shutil.make_archive(
        os.path.join(args.out, "Linux", f"QtDemo-{version}-linux-x86_64"),
        "gztar", stage)
    print(f"[Linux] 产物: {tar_path}")
    return tar_path


# ============================================================
#  入口
# ============================================================

def main():
    parser = argparse.ArgumentParser(description="跨平台打包脚本（Qt6 + CMake + Conan）")
    parser.add_argument("-t", "--target", default="all",
                        help="打包目标，逗号分隔: all/qt/server/console（默认 all）")
    parser.add_argument("--build-type", default="Release", choices=["Release", "Debug"])
    parser.add_argument("--data-source", default="sample",
                        choices=["sample", "http", "proto"],
                        help="数据源: sample/http/proto（默认 sample）")
    parser.add_argument("--qt-dir", default=detect_qt_dir(),
                        help="Qt 安装根目录（默认自动探测或 QT_DIR）")
    parser.add_argument("--out", default=os.path.join(BUILD_DIR, "dist"),
                        help="产物输出根目录（默认 build/dist）")
    parser.add_argument("--skip-build", action="store_true",
                        help="跳过 conan install + cmake，直接打包现有产物")
    args = parser.parse_args()

    if not args.qt_dir or not os.path.isdir(args.qt_dir):
        raise SystemExit(f"未找到 Qt 安装目录，请用 --qt-dir 指定（当前: {args.qt_dir!r}）")
    if args.data_source == "proto" and args.build_type != "Release":
        print("[警告] protobuf 数据源建议使用 Release 构建")

    version = parse_version()
    print(f"项目版本: {version}  平台: {SYSTEM}  Qt: {args.qt_dir}")

    step_build(args)

    if SYSTEM == "Darwin":
        package_macos(args, version)
    elif SYSTEM == "Windows":
        package_windows(args, version)
    elif SYSTEM == "Linux":
        package_linux(args, version)
    else:
        raise SystemExit(f"不支持的操作系统: {SYSTEM}")

    print("\n===== 打包完成 =====")


if __name__ == "__main__":
    main()