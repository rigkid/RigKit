import stage

_s = {"frames": 0}


def update(dt):
    _s["dt"] = dt
    _s["frames"] += 1
    if _s["frames"] == 1:
        ball = stage.find("ball")
        if ball is None:
            raise RuntimeError("ball missing")
        if not stage.sim_owns(ball):
            raise RuntimeError("ball should be simulated")
        try:
            stage.set_position(ball, 0.0, 9.0, 0.0)
        except RuntimeError as exc:
            if "sim owns pose" not in str(exc):
                raise
        else:
            raise RuntimeError("set_position on a rigid body must fail")
        vx, vy, vz = stage.velocity(ball)
        if vx < 0.3:
            raise RuntimeError("ball launch velocity")
        if stage.name(ball) != "ball":
            raise RuntimeError("ball name")
        if stage.count() < 1:
            raise RuntimeError("count")
        dropped = stage.spawn("drop", 1.1, 3.5, 0.2, 0.2, 0.0, 0.0)
        dropped_name = stage.name(dropped)
        if not dropped_name.startswith("drop-"):
            raise RuntimeError(dropped_name)
        marker = stage.create("marker")
        stage.set_position(marker, 0.0, 1.2, -2.0)
        temp = stage.create("temp")
        stage.destroy(temp)
        if stage.find("temp") is not None:
            raise RuntimeError("temp survived")
        px, py, pz = stage.position(marker)
        if abs(px) > 0.001 or abs(py - 1.2) > 0.001 or abs(pz + 2.0) > 0.001:
            raise RuntimeError("marker pose")
        _s["marker"] = marker
    elif _s["frames"] == 8:
        stage.set_position(_s["marker"], 0.4, 1.2, -2.0)
