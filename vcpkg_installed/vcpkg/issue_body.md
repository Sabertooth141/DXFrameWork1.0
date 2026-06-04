Package: zlib:x64-windows@1.3.2#1

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35227.0
- CMake Version: 4.3.2
-    vcpkg-tool version: 2026-05-27-d5b6777d666efc1a7f491babfcdab37794c1ae3e
    vcpkg-scripts version: 81de677151 2026-06-04 (2 hours ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
Downloading https://github.com/madler/zlib/archive/v1.3.2.tar.gz -> madler-zlib-v1.3.2.tar.gz
Successfully downloaded madler-zlib-v1.3.2.tar.gz
-- Extracting source C:/vcpkg/downloads/madler-zlib-v1.3.2.tar.gz
-- Applying patch 0001-no-version-script-on-ohos.patch
-- Using source at C:/vcpkg/buildtrees/zlib/src/v1.3.2-c19a0bfd23.clean
-- Configuring x64-windows
-- Building x64-windows-dbg
-- Building x64-windows-rel
-- Fixing pkgconfig file: C:/vcpkg/packages/zlib_x64-windows/lib/pkgconfig/zlib.pc
Downloading msys2-mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst, trying https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
Attempt 1 of 3, retrying download.
Attempt 2 of 3, retrying download.
error: Download timed out.
error: Download timed out.
error: Reached maximum number of attempts, won't retry download from https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst.
Trying https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
Attempt 1 of 3, retrying download.
Trying https://mirror.yandex.ru/mirrors/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
Trying https://mirrors.tuna.tsinghua.edu.cn/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
Trying https://mirrors.ustc.edu.cn/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
Trying https://mirror.selfnet.de/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
error: Download timed out.
error: curl operation failed with error code 7 (Could not connect to server).
error: Not a transient network error, won't retry download from https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
error: curl operation failed with error code 6 (Could not resolve hostname).
error: Not a transient network error, won't retry download from https://mirror.yandex.ru/mirrors/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
error: curl operation failed with error code 6 (Could not resolve hostname).
error: Not a transient network error, won't retry download from https://mirrors.tuna.tsinghua.edu.cn/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
error: curl operation failed with error code 6 (Could not resolve hostname).
error: Not a transient network error, won't retry download from https://mirrors.ustc.edu.cn/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
error: curl operation failed with error code 6 (Could not resolve hostname).
error: Not a transient network error, won't retry download from https://mirror.selfnet.de/msys2/mingw/mingw64/mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
note: If you are using a proxy, please ensure your proxy settings are correct.
Possible causes are:
1. You are actually using an HTTP proxy, but setting HTTPS_PROXY variable to `https://address:port`.
This is not correct, because `https://` prefix claims the proxy is an HTTPS proxy, while your proxy (v2ray, shadowsocksr, etc...) is an HTTP proxy.
Try setting `http://address:port` to both HTTP_PROXY and HTTPS_PROXY instead.
2. If you are using Windows, vcpkg will automatically use your Windows IE Proxy Settings set by your proxy software. See: https://github.com/microsoft/vcpkg-tool/pull/77
The value set by your proxy might be wrong, or have same `https://` prefix issue.
3. Your proxy's remote server is out of service.
If you believe this is not a temporary download server failure and vcpkg needs to be changed to download this file from a different location, please submit an issue to https://github.com/Microsoft/vcpkg/issues
CMake Error at scripts/cmake/vcpkg_download_distfile.cmake:136 (message):
  Download failed, halting portfile.
Call Stack (most recent call first):
  scripts/cmake/vcpkg_acquire_msys.cmake:25 (vcpkg_download_distfile)
  scripts/cmake/vcpkg_acquire_msys.cmake:124 (z_vcpkg_acquire_msys_download_package)
  scripts/cmake/vcpkg_acquire_msys.cmake:212 (z_vcpkg_acquire_msys_download_packages)
  scripts/cmake/vcpkg_find_acquire_program(PKGCONFIG).cmake:42 (vcpkg_acquire_msys)
  scripts/cmake/vcpkg_find_acquire_program.cmake:144 (include)
  scripts/cmake/vcpkg_fixup_pkgconfig.cmake:193 (vcpkg_find_acquire_program)
  ports/zlib/portfile.cmake:26 (vcpkg_fixup_pkgconfig)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "dxpractice",
  "version": "0.1.0",
  "dependencies": [
    "assimp"
  ]
}

```
</details>
