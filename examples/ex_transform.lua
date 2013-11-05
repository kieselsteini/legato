local al = legato.al

--[[ main ]]--
local filename = '/data/mysha.pcx'

local display = al.create_display(640, 480)
local subbitmap = al.create_sub_bitmap(al.get_backbuffer(display), 50, 50, 640 - 50, 480 - 50)
local overlay = al.create_sub_bitmap(al.get_backbuffer(display), 100, 100, 300, 50)

al.set_window_title(display, filename)
local bitmap = assert(al.load_bitmap(filename))
local font = assert(al.create_builtin_font())

al.set_new_bitmap_flags({memory_bitmap = true})
local buffer = al.create_bitmap(640, 480)
local buffer_subbitmap = al.create_sub_bitmap(buffer, 50, 50, 640 - 50, 480 - 50)
local soft_font = al.create_builtin_font()

local timer = al.create_timer(1.0 / 60.0)
local queue = al.create_event_queue()
al.register_event_source(queue, al.create_keyboard_state())
al.register_event_source(queue, display)
al.register_event_source(queue, timer)
al.start_timer(timer)

local w, h = al.get_bitmap_size(bitmap)
al.set_target_bitmap(overlay)
local transform = al.create_transform()
al.identity_transform(transform)
al.rotate_transform(transform, -0.06)
al.use_transform(transform)

local redraw = false
local blend = false
local software = false
local use_subbitmap = false

while true do
    local event = al.wait_for_event(queue)
    if event then
        if event.type == 'display_close' then
            return
        elseif event.type == 'timer' then
            redraw = true
        elseif event.type == 'key_down' then
            if event.keycode == 12 then
                blend = not blend
            elseif event.keycode == 2 then
                use_subbitmap = not use_subbitmap
            elseif event.keycode == 19 then
                software = not software
                if software then
                    local t = al.create_transform()
                    al.identity_transform(t)
                    al.use_transform(t)
                end
            elseif event.keycode == 59 then
                return
            else
                print(event.keycode, collectgarbage('count'))
            end
        end
    end
    if redraw and al.is_event_queue_empty(queue) then
        redraw = false
        local t = 3.0 + al.get_time()
        al.set_blender('add', 'one', 'one')
        local tint
        if blend then
            tint = al.map_rgb_f(0.5, 0.5, 0.5, 0.5)
        else
            tint = al.map_rgb_f(1, 1, 1, 1)
        end
        if software then
            if use_subbitmap then
                al.set_target_bitmap(buffer)
                al.clear_to_color(al.map_rgb_f(1, 0, 0))
                al.set_target_bitmap(buffer_subbitmap)
            else
                al.set_target_bitmap(buffer)
            end
        else
            if use_subbitmap then
                al.set_target_backbuffer(display)
                al.clear_to_color(al.map_rgb_f(1, 0, 0))
                al.set_target_bitmap(subbitmap)
            else
                al.set_target_backbuffer(display)
            end
        end

        -- Set the transformation on the target bitmap
        al.identity_transform(transform)
        al.translate_transform(transform, -640 / 2, -480 / 2)
        al.scale_transform(transform, 0.15 + math.sin(t / 5), 0.15 + math.cos(t / 5))
        al.rotate_transform(transform, t / 50)
        al.translate_transform(transform, 640 / 2, 480 / 2)
        al.use_transform(transform)

        -- Draw some stuff
        al.clear_to_color(al.map_rgb_f(0, 0, 0))
        al.draw_tinted_bitmap(bitmap, tint, 0, 0, 0)
        al.draw_tinted_scaled_bitmap(bitmap, tint, w / 4, h / 4, w / 2, h / 2, w, 0, w / 2, h / 4, 0)
        al.draw_tinted_bitmap_region(bitmap, tint, w / 4, h / 4, w / 2, h / 2, 0, h, {flip_vertical = true})
        al.draw_tinted_scaled_rotated_bitmap(bitmap, tint, w / 2, h / 2, w + w / 2, h + h / 2, 0.7, 0.7, 0.3, 0)
        al.draw_pixel(w + w / 2, h + h / 2, al.map_rgb_f(0, 1, 0))
        al.put_pixel(w + w / 2 + 2, h + h / 2 + 2, al.map_rgb_f(0, 1, 1))
        al.draw_circle(w, h, 50, al.map_rgb_f(1, 0.5, 0), 3)

        al.set_blender('add', 'one', 'zero')
        if software then
            al.draw_text(soft_font, al.map_rgb_f(1, 1, 1, 1), 640 / 2, 430, "Software Rendering", {centre = true})
            al.set_target_backbuffer(display)
            al.draw_bitmap(buffer, 0, 0, 0)
        else
            al.draw_text(font, al.map_rgb_f(1, 1, 1, 1), 640 / 2, 430, "Hardware Rendering", {centre = true})
        end

        -- Each target bitmap has its own transformation matrix, so this
        -- overlay is unaffected by the transformations set earlier.
        al.set_target_bitmap(overlay)
        al.set_blender('add', 'one', 'one')
        al.draw_text(font, al.map_rgb_f(1, 1, 0, 1), 0, 10, "hello!")

        al.set_target_backbuffer(display)
        al.flip_display()
    end
end

