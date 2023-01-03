local Player = require("player")
local Dialogue = require("dialogue")

return class {
    name = "test"
    tmx = "test.tmx"
    player = null
    anim = null
    dialogue = null

    constructor() {}

    function on_load() {
        // Animation test
        /*
        anim = Animation({
            json = "graphics/sprites/player.json"
            pos = [200.0, 200.0]
        })
        anim.set_state("Dance");
        */

        // Font rendering test
        local font = Font("fonts/apple_kid.fnt");
        local text = Text({
            font = font
            pos = [10, 10]
            color = color_from_hex("#000000")
            contents = "Hello world!\nThe quick brown fox jumps over the lazy dog."
        })

        // TODO: get object reference from Tiled
        // player = tiled_loadobject("Player/player")

        // Sound test
        // local audio_source = Sound.load_ogg("audio/osohe-dance-test.ogg")
        // local audio_inst = Sound.make_instance(audio_source)
        // Sound.play(audio_inst)

        // Dialogue
        dialogue = Dialogue()
        dialogue.run()
    }

    function on_update(dt) {
        if (!dialogue.get_active()) {
            // player.update()
        }

        dialogue.update()
    }
    function on_render() {
    }
}