# FFmpegSharedLibraries
用于构建 FFmpeg 共享库运行时的 GitHub Actions 工作流，启用 `libuavs3d`，并在 GPL 构建中启用 `libdavs2`（来自 `davs2-10bit`）。
## 构建产物
- `macos-14`：arm64 `.dylib`
- `windows-latest`：win64 `.exe`
- `ubuntu-latest`：linux x64 `.so`
每个工作流仅构建 FFmpeg 共享库。
- `libuavs3d` 始终从源码构建，作为静态依赖链接进 FFmpeg。
- `libdavs2`（`davs2-10bit`）从源码构建，当 `license_flavor=gpl` 时静态链接。
- 运行时产物不包含独立的 `libuavs3d`/`libdavs2` 动态库。
- macOS 运行时启用 VideoToolbox 硬件解码。
- Windows 运行时通过 Windows 系统API启用 D3D11VA 和 DXVA2 硬件解码，无需额外的运行时 DLL。
构建过程会将第三方源码下载到工作流的临时工作根目录（GitHub Actions 中为 `$RUNNER_TEMP/ffmpeg-runtime-build`），并对 `davs2-10bit` 和 FFmpeg 应用一组本地补丁：
- `patches/davs2-10bit/0001-enable-10bit-build-and-propagate-frame-packet-position.patch`
  - 启用 10 位 `davs2-10bit` 构建，
  - 将解码器侧的包文件位置元数据通过 `libdavs2` 输出传播。
- `patches/davs2-10bit/0002-x86-build-avx-codepaths-as-dispatch-only.patch`
  - 使通用 `x86_64` 目标文件保持在 SSE4.x 基线，而不是整个库使用 `-mavx` 编译，
  - 将 AVX 和 AVX2 转换单元单独编译，使运行时 CPUID 分发同时兼容新旧 x86 设备。
- `patches/davs2-10bit/0003-fix-dpb-stale-ref-frames.patch`
  - 修复某些 AVS2 流中可能出现的 DPB（解码图像缓冲区）分配循环挂起问题，
  - 回收 `b_refered_by_others==1 && i_ref_count==0` 的陈旧参考帧（RPS 从未清理的“僵尸帧”），而不是让管线永远阻塞。
- [`patches/ffmpeg/0001-avs-dra-runtime.patch`](./patches/ffmpeg/0001-avs-dra-runtime.patch)
  - 引入现有的原生 AVS+/DRA 解码器，并将其集成移植到 FFmpeg 9，
  - 保留上游 9.x CAVS 解析器的序列头解析，
  - 修复 CAVS 数据包在异步解码和帧重排序过程中的来源追踪，
  - 为 AVS+/AVS2/AVS3 导出可选的解码器侧字节位置，而不恢复已移除的 `AVFrame.pkt_pos` ABI 成员。
- [`patches/ffmpeg/0002-av3a-demux-probing.patch`](./patches/ffmpeg/0002-av3a-demux-probing.patch)
  - 在原始流和 MPEG-TS 中识别 AV3A（Audio Vivid），包括在节目表到达之前，
  - 无需 AV3A 解码器 SDK 即可读取音频参数，探测和解析器缓冲均设有上限。
- [`patches/ffmpeg/0003-av3a-container-remux.patch`](./patches/ffmpeg/0003-av3a-container-remux.patch)
  - 增加 AV3A MP4 配置读写、MPEG-TS 信令及原始流输出，
  - 无需 AV3A 解码器或编码器 SDK 即可保留压缩数据包。
参见 [`patches/ffmpeg/README.md`](./patches/ffmpeg/README.md) 了解来源出处、位置元数据、分配行为及解码器特定的 opaque 约定。
## 工作流输入
所有构建工作流均固定为：
- `ffmpeg_version`：`9.0.2`
- `license_flavor`：`gpl`
手动运行使用 `workflow_dispatch`，不提供自定义输入字段。
构建脚本仅接受 FFmpeg `9.0.x`。构建并发数默认最多四个任务，以限制编译器峰值内存；可通过设置 `BUILD_JOBS` 覆盖。
## FFmpeg 9 ABI
| 库 | ABI 主版本 |
| --- | ---: |
| avutil | 61 |
| avcodec | 63 |
| avformat | 63 |
| avdevice | 63 |
| avfilter | 12 |
| swscale | 10 |
| swresample | 7 |
`libpostproc` 已在上游被移除，不再打包。这些库需要 FFmpeg 9 绑定，无法替换当前的 TSCutter 7.x 运行时。
macOS 打包会将依赖路径重写为 `@loader_path`，并在重写后更新临时签名，使 ARM64 库能够从扁平化包中加载。
## 第三方库
- FFmpeg：https://ffmpeg.org/
- uavs3d：https://github.com/uavs3/uavs3d
- davs2-10bit：https://github.com/xatabhk/davs2-10bit
- AV3A 解复用/复用、AATF 表和头文件：[OpenHarmony third_party_ffmpeg](https://github.com/openharmony/third_party_ffmpeg)（原始容器实现：[ac883016](https://github.com/openharmony/third_party_ffmpeg/commit/ac883016d94bb95354e2824a501702f64c9ad945)，Shuai Liu，2024-11-01）
- ffmpeg_cavs_dra 补丁来源：https://github.com/maliwen2015/ffmpeg_cavs_dra
## 工作流
- [`.github/workflows/build-ffmpeg-runtime-macos.yml`](./.github/workflows/build-ffmpeg-runtime-macos.yml)
- [`.github/workflows/build-ffmpeg-runtime-windows.yml`](./.github/workflows/build-ffmpeg-runtime-windows.yml)
- [`.github/workflows/build-ffmpeg-runtime-linux.yml`](./.github/workflows/build-ffmpeg-runtime-linux.yml)
