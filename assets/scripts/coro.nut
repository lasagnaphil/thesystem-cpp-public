local function wait_frames(frames) {
    for (local i = 0; i < frames; i++) {
        suspend()
    }
}

local function wait_seconds(seconds) {
    local t = 0.0
    do {
        t += app_frame_duration()
        suspend()
    } while (t < seconds)
}

local function wait_key_pressed(scancode) {
    do {
        suspend()
    } while (!Input.is_key_pressed(scancode))
}

local function wait_key_entered(scancode) {
    do {
        suspend()
    } while (!Input.is_key_entered(scancode))
}

local function wait_key_exited(scancode) {
    do {
        suspend()
    } while (!Input.is_key_exited(scancode))
}

local function wait_frames_until_key_pressed(frames, scancode) {
    for (local i = 0; i < frames; i++) {
        suspend()
        if (Input.is_key_entered(scancode)) return true
    }
    return false
}

local function wait_frames_until_key_entered(frames, scancode) {
    for (local i = 0; i < frames; i++) {
        suspend()
        if (Input.is_key_entered(scancode)) return true
    }
    return false
}

local function wait_frames_until_key_exited(frames, scancode) {
    for (local i = 0; i < frames; i++) {
        suspend()
        if (Input.is_key_entered(scancode)) return true
    }
    return false
}


return {
    wait_frames = wait_frames,
    wait_seconds = wait_seconds
    wait_key_pressed = wait_key_pressed
    wait_key_entered = wait_key_entered
    wait_key_exited = wait_key_exited
    wait_frames_until_key_pressed = wait_frames_until_key_pressed
    wait_frames_until_key_entered = wait_frames_until_key_entered
    wait_frames_until_key_exited = wait_frames_until_key_exited
}