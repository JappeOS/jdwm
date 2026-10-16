# Surface buffer lifetime regression

From the repository root:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror -Inative/src \
  native/tests/surface_buffer_chain_test.cpp -o /tmp/surface_buffer_chain_test
/tmp/surface_buffer_chain_test
```

The test covers overlapping Flutter frame leases, delayed release of an older
frame, and a frame that outlives its surface's buffer chain.

For runtime texture diagnostics, set `ZENITH_TRACE_TEXTURES=1` when launching
the rebuilt compositor. Each external texture callback logs the surface ID,
GL texture name/target, actual buffer size, requested widget size, and whether
the texture is visible in Flutter's shared GL context (`shared=1`). Registration
failures and missing GLES buffers are logged without this flag.

Check a Wayland SHM client and a GPU client, then resizing, closing, buffer
scaling, and viewport cropping. These checks require a running compositor;
the standalone test does not verify rendered pixels.
