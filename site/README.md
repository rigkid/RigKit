# RigKit site

Static landing for GitHub Pages. CI copies this tree to the Pages root and Doxygen HTML to `api/`.

| URL | Content |
|-----|---------|
| [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/) | This landing |
| [https://rigkid.github.io/rigkit/api/](https://rigkid.github.io/rigkit/api/) | Host aggregate API (Doxygen) |

Background is a WebGL vector-field shader (`field.js`) — seed / scale / hue / warp roll on each load; pointer position warps the field. Respects `prefers-reduced-motion`. Workflow: [`.github/workflows/docs.yml`](../.github/workflows/docs.yml).
