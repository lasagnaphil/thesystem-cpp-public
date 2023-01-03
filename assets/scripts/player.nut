local string = require("string")
local math = require("math")
local dump = require("dump")

local class Player extends ScriptableKinematicBody {
    speed = 100.0

    constructor(args) {
        base.constructor({ margin = 0.1 })
        set_pos(args.pos)
        args.pos = [0, 0]
        local sprite = Sprite(args)
        add_child(sprite)
    }

    function update(dt) {
        local x = get_pos_x()
        local y = get_pos_y()
        local dx = 0.0
        local dy = 0.0
        if (Input.is_key_pressed(Scancode.Left)) {
            dx -= 1.0
        }
        if (Input.is_key_pressed(Scancode.Right)) {
            dx += 1.0
        }
        if (Input.is_key_pressed(Scancode.Up)) {
            dy -= 1.0
        }
        if (Input.is_key_pressed(Scancode.Down)) {
            dy += 1.0
        }
        local l = math.sqrt(dx*dx + dy*dy)
        if (l > 0.0) {
            dx = speed * dx * dt / l
            dy = speed * dy * dt / l
            move_and_slide([dx, dy])
        }
    }

    function print_pos() {
        string.printf("Player position is (%d, %d)", x, y)
    }
}

return Player