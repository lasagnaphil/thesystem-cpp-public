local math = require("math")

local function random_float() {
    return math.rand() / math.RAND_MAX.tofloat()
}

local class Bunny extends ScriptableSprite {
    gravity = 0.75
    x = 0
    y = 0
    speed_x = 0
    speed_y = 0
    bounds = null

    constructor(texture, _bounds, _x, _y) {
        base.constructor({
            tex = texture,
            srcrect = [0, 0, 32, 64],
            pos = [100, 100],
            scale = [1, 1],
            layer = Layers.Sprite
        })
        bounds = _bounds
        x = _x
        y = _y
        speed_x = 10.0 * random_float()
        speed_y = 10.0 * random_float() - 5.0
    }

    function update(dt) {
        local x = get_pos_x()
        local y = get_pos_y()
        x += speed_x
        y += speed_y
        speed_y += gravity

        if (x > bounds.right) {
            speed_x *= -1.0
            x = bounds.right
        }
        else if (x < bounds.left) {
            speed_x *= -1.0
            x = bounds.left
        }
        if (y > bounds.bottom) {
            speed_y *= -0.85
            y = bounds.bottom
            if (random_float() > 0.5) {
                speed_y -= 6 * random_float()
            }
        }
        else if (y < bounds.top) {
            speed_y = 0
            y = bounds.top
        }
        set_pos_x(x)
        set_pos_y(y)
    }
}

return class {
    name = "bunnymark"

    texture_filenames = [
        "graphics/sprites/bunnymark/rabbitv3_ash.png",
        "graphics/sprites/bunnymark/rabbitv3_batman.png",
        "graphics/sprites/bunnymark/rabbitv3_bb8.png",
        "graphics/sprites/bunnymark/rabbitv3_neo.png",
        "graphics/sprites/bunnymark/rabbitv3_sonic.png",
        "graphics/sprites/bunnymark/rabbitv3_spidey.png",
        "graphics/sprites/bunnymark/rabbitv3_stormtrooper.png",
        "graphics/sprites/bunnymark/rabbitv3_superman.png",
        "graphics/sprites/bunnymark/rabbitv3_tron.png",
        "graphics/sprites/bunnymark/rabbitv3_wolverine.png",
        "graphics/sprites/bunnymark/rabbitv3.png",
        "graphics/sprites/bunnymark/rabbitv3_frankenstein.png"
    ]
    textures = []

    bounds = {
        left = 0.0
        top = 0.0
        right = 800.0
        bottom = 600.0
    }

    bunnies = []

    start_bunny_count = 0

    is_adding = false
    max_count = 200000
    amount = 100
    stat_text = null

    constructor() {}

    function on_load() {
        local font = Font("fonts/apple_kid.fnt");
        stat_text = Text({
            font = font
            pos = [10, 10]
            color = color_from_hex("#FFFFFF")
            contents = "FPS: "
            layer = Layers.UI
        })

        foreach (filename in texture_filenames) {
            textures.append(Texture(filename))
        }

        if (start_bunny_count > 0) {
            add_bunnies(start_bunny_count)
        }
    }

    function on_update(dt) {
        if (Input.is_key_entered(Scancode.Enter)) {
            is_adding = true
        }
        if (Input.is_key_exited(Scancode.Enter)) {
            is_adding = false
        }
        if (is_adding) {
            if (bunnies.len() < max_count) {
                add_bunnies(amount)
            }
        }
        // local fps = app_get_fps()
        // stat_text.update_text($"FPS: {fps}")
        stat_text.set_contents($"FPS: {app_fps()}, Count: {bunnies.len()}")
    }

    function on_render() {
        // print("Rendering...")
    }

    function add_bunnies(num) {
        for (local i = 0; i < num; i++) {
            local texture = textures[bunnies.len() % textures.len()]
            local bunny = Bunny(texture, bounds, (bunnies.len() % 2) * 800.0, 0.0)
            bunnies.append(bunny)
        }
    }
}