# Namespaces

## `rigkit`

Default for host and pack C++ (`IApp`, managers, pack entry types, renderers, `IMui` fulfillments). RigKit is a coded **Rig** fulfillment; the C++ namespace stays `rigkit`.

## `rigkit::ecs`

POD components and host ECS systems (`CTransform`, `CMesh`, `SEvent`, ...) - host `src/ecs/` and data packs such as **rigComponent**. One namespace for both; pack boundary is data vs code (`rigComponent` / `rigSystems`), not a second C++ namespace.

Portable field layouts when shared across hosts: [Rig schemas](https://github.com/rigkid/RigWorks/tree/main/schemas) (zero code).

## `rig`

Artist helpers that write POD (`makeRect`, `makeMesh`, ...) in **rigComponent**. Short on purpose; not under `rigkit::`.

## Does not use sub-namespaces

UI and rendering live in `rigkit` (and pack folders / file layout). There is no `rigkit::ui` or `rigkit::rendering`.

## Example

```cpp
namespace rigkit {
namespace ecs {
struct CTransform { /* ... */ };
void SShapeRendering(/* ... */);
} // namespace ecs
} // namespace rigkit

namespace rig {
auto makeRect(/* ... */);
}
```

New types: put components/systems in `rigkit::ecs`, everything else in `rigkit` unless it is an artist helper (`rig`). Document a new sub-namespace here if you add one.
