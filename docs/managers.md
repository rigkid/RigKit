# Core Managers in RigKit

This document outlines the main manager classes in the RigKit framework, their responsibilities, and how to access and use them.

## Access Pattern
- All managers are owned by the main engine class (`RigKitEngine`).
- Access managers via the engine instance:

```cpp
rigkit::RigKitEngine* engine = ...;
rigkit::MEcs* ecs = engine->getECSManager();
rigkit::MPack* packs = engine->getPackManager();
rigkit::Mui* ui = engine->getUIManager();
rigkit::MRendering* rendering = engine->getRenderingManager();
rigkit::MCanvas* canvas = engine->getCanvasManager();
rigkit::MSettings* settings = engine->getSettingsManager();
```

### Inside `rigkit::IApp` subclasses
When you derive from `rigkit::IApp`, the framework automatically injects an engine pointer before `setup()` is called.  The base `IApp` class then exposes convenience accessors so you can use the same managers without boiler-plate:

```cpp
class MyCoolApp : public rigkit::IApp {
    void setup(rigkit::RigKitEngine* engine) override {
        // No need to cache the engine – the base class already did.
        auto* ecs       = getECSManager();
        auto* ui        = getUIManager();
        auto* rendering = getRenderingManager();
        // ...
    }
    // ...
};
```

These helpers (`getECSManager()`, `getPackManager()`, etc.) are protected members of `IApp`, keeping the public surface of your app class clean while still granting easy access to core managers.

Outside of an `IApp` subclass (e.g. in standalone utilities or engine components), continue to obtain managers through an explicit `rigkit::RigKitEngine*` pointer as shown above.

## Manager List

### ECS Manager
- **Purpose:** Entity-Component-System (ECS) registry and logic.
- **Access:** `engine->getECSManager()`

### Pack Manager
- **Purpose:** Manages discovery, loading, and hooks of packs.
- **Access:** `engine->getPackManager()`

### UI Manager
- **Purpose:** Handles UI logic, ImGui integration, and window management.
- **Access:** `engine->getUIManager()`

### Canvas Manager (MCanvas)
- **Purpose:** Manages canvases, their creation, and switching.
- **Access:** `engine->getCanvasManager()`
- **Note:** Internally uses `MRendering` for rendering operations.

### Rendering Manager
- **Purpose:** Manages renderers, graphics, and low-level rendering logic. Used by `MCanvas` and available for advanced use cases.
- **Access:** `engine->getRenderingManager()`

### Settings Manager
- **Purpose:** User preference registry (pack/host POD sections) and `rigkit_settings.json` load/save. See [settings.md](settings.md).
- **Access:** `engine->getSettingsManager()`

## Example Usage

```cpp
// Accessing managers from within a class with an engine pointer
void MyClass::doSomething() {
    auto* ecs = m_engine->getECSManager();
    auto* packs = m_engine->getPackManager();
    // ...
}
```

---

**Note:**
- If you need a manager in a class, pass a pointer/reference to it or to the engine.
- This pattern improves testability and flexibility compared to singletons. 
