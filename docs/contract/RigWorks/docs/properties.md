# Properties and Datatypes

Portable **property descriptors** let any properties surface show any opt-in component field.

This is Contract vocabulary (zero code).

## Why

ECS components are POD. Editors should not hardcode one inspector per type forever. A component advertises rows: name + **datatype** + value. A property manager that understands the datatype table can draw anything that opts in.

## Property row

| Field | Meaning |
|-------|---------|
| `id` | Stable index within the component |
| `name` | Display label |
| `type` | Datatype id (see below) |
| `value` | The field itself (host binds pointer / path / leaf) |

Exact host types are fulfillment. The row meaning is Contract.

## Datatype ids

| Id | Meaning |
|----|---------|
| `bool` | Boolean |
| `int` | Signed integer |
| `uint` | Unsigned integer |
| `float` | 32-bit float |
| `double` | 64-bit float |
| `string` | Text |
| `vec2` | Two floats |
| `vec3` | Three floats |
| `vec4` | Four floats |
| `quat` | Four floats (x, y, z, w) |
| `entity` | Entity id (host integer / handle encoding) |
| `enum` | Named choice; schema lists the literals |

Domain meaning (colour, pitch, …) lives in **schema field names**, not extra type ids. A host may draw a `vec4` named `rgba` with a colour control — fulfillment, not a Contract type.

Node graph pins and params reuse this table — see [`rig.node.pin`](../schemas/node/pin.md) / [`rig.node.param`](../schemas/node/param.md). Group interface pins ([`rig.node.publish`](../schemas/node/publish.md)) map outer pins to interior pins; the outer pin’s `type` is still one of these datatype ids.

Arrays and nested structs are schema-local until a later properties version. Property managers skip or stub unknown types without crashing.

Hosts may add prefixed ids.

## Rules

- Components that should appear in Properties advertise these rows (or an equivalent catalog).
- No toolkit types inside the descriptor.
- One type per field.

## Fulfillment (RigKit)

RigKit maps this table to `propTypes` / `sProp` / `GetProperties()`. **rigImGui** draws those rows in Properties.

See [ui.md](ui.md), [ecs.md](ecs.md).
