# rigPsd

Leaf PSD IO: 8-bit RGB layered read and write. Format model only - mapping onto `CPixelLayer` lives in **rigPixelPlotter**.

Does not model text, smart objects, adjustment layers, or ABR. Unknown Photoshop tagged blocks are skipped on read and not preserved on write.

## Use

```cpp
#include "PsdFile.h"

rigkit::psd::PsdFile file;
rigkit::psd::readFile("in.psd", file);
rigkit::psd::writeFile("out.psd", file);
```

Pixel hosts call `PixelDoc::loadPsd` / `savePsd` when this pack is linked.

[API/docs](https://rigkid.github.io/rigPsd/)
