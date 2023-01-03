local coro = require("coro")

local class Dialogue {
	text = null

	sprite_box = null
	sprite_portrait = null

	coroutine = null

	is_active = false
	is_portrait_active = false
	is_choice_active = false

	margin = 20

	constructor() {
        local font = Font("fonts/apple_kid.fnt");
        local width = app_width()
        local height = app_height()
		text = Text({
			font = font
			pos = [20, 20]
			color = color_from_hex("#000000")
			contents = ""
			layer = Layers.UI
			z_index = 2
			enabled = false
		})
		sprite_box = Sprite({
			tex = Texture("graphics/sprites/dialogue_box.png")
			pos = [0, height-112]
			layer = Layers.UI
			z_index = 0
			enabled = false
		})
		sprite_portrait = Sprite({
			tex = Texture("graphics/sprites/portrait_placeholder.png")
			pos = [20, 20]
			layer = Layers.UI
			z_index = 1
			enabled = false
		})
		sprite_box.add_child(text)
		sprite_box.add_child(sprite_portrait)
        coroutine = newthread(script.bindenv(this))
	}

	function set_text(str) {
		text.set_contents(str)
	}

	function run() {
		coroutine.call()
		is_active = true
	}

	function update() {
		if (is_active) {
			coroutine.wakeup()
		}
	}

	function get_active() { 
		return is_active
	}

	function say(line) {
		set_text("")
		coro.wait_frames(3)
		for (local i = 1; i <= line.len(); i++) {
			if (coro.wait_frames_until_key_entered(3, Scancode.X)) {
				set_text(line)
				break
			}
			set_text(line.slice(0, i))
		}
		coro.wait_key_entered(Scancode.X)
	}

	function disable_portrait() {
		is_portrait_active = false
		sprite_portrait.set_enable(false)
		text.set_pos_x(margin)
	}

	function enable_portrait(dir = "left") {
		local width = app_width()
        local height = app_height()
		is_portrait_active = true
		sprite_portrait.set_enable(true)
		if (dir == "left") {
			sprite_portrait.set_pos_x(margin)
			text.set_pos_x(margin + sprite_portrait.get_width() + margin)
		}
		else if (dir == "right") {
			sprite_portrait.set_pos_x(sprite_box.get_width() - sprite_portrait.get_width() - margin)
			text.set_pos_x(margin)
		}
		else {
			error("enable_portrait(dir) supports only left/right dir")
		}
	}

	function dialogue_start() {
		is_active = true
		text.set_enable(true)
		sprite_box.set_enable(true)
		disable_portrait()
	}

	function dialogue_end() {
		is_active = false
		text.set_enable(false)
		sprite_box.set_enable(false)
		sprite_portrait.set_enable(false)
	}

	function script() {
		dialogue_start()
		disable_portrait()
        say("Hi, player! This is some sample text.")
        enable_portrait("left")
        say("Here is some more sample text.")
        enable_portrait("right")
        say("Here's another one!")
        dialogue_end()
	}
}

return Dialogue