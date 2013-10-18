--[[ ex_ttf ]]--
local al = legato.al

local font_file = '/data/DejaVuSans.ttf'
local ex = {}

-- rendering
function render()
    local white = al.map_rgb_f(1, 1, 1)
    local black = al.map_rgb_f(0, 0, 0)
    local red = al.map_rgb_f(1, 0, 0)
    local green = al.map_rgb_f(0, 1, 0)
    local blue = al.map_rgb_f(0, 0, 1)

    al.clear_to_color(white)
    al.hold_bitmap_drawing(true)

    al.draw_text(ex.f1, black, 50, 50, 'Tulip (kerning)')
    al.draw_text(ex.f2, black, 50, 100, 'Tulip (no kerning)')
    al.draw_text(ex.f3, black, 50, 200, 'This font has a size of 12 pixels, the one above has 48 pixels')

    al.hold_bitmap_drawing(false)
    al.hold_bitmap_drawing(true)

    al.draw_text(ex.f3, red, 50, 220, 'The color can be simply changed')

    al.hold_bitmap_drawing(false)
    al.hold_bitmap_drawing(true)

    ex.f3:draw_text(green, 50, 240, 'Some unicode symbols')
    ex.f3:draw_text(green, 50, 260, ex.config:get_value('', 'symbols1'))
    ex.f3:draw_text(green, 50, 280, ex.config:get_value('', 'symbols2'))
    ex.f3:draw_text(green, 50, 300, ex.config:get_value('', 'symbols3'))

    al.draw_text(ex.f5, black, 50, 420, 'forced monochrome')

    al.hold_bitmap_drawing(false)

    local target_w, target_h = al.get_target_bitmap():get_size()

    local xpos = target_w - 10
    local ypos = target_h - 10
    local x, y, w, h = al.get_text_dimensions(ex.f4, 'Legato')
    local as = al.get_font_ascent(ex.f4)
    local de = al.get_font_descent(ex.f4)
    xpos = xpos - w
    ypos = ypos - h
    x = x + xpos
    y = y + ypos

    al.draw_rectangle(x, y, x + w, y + h, black)
    al.draw_line(x, y + as, x + w, y + as, black)
    al.draw_line(x, y + as + de, x + w, y + as + de, black)
    
    al.hold_bitmap_drawing(true)
    al.draw_text(ex.f4, blue, xpos, ypos, 'Legato')
    al.hold_bitmap_drawing(false)

    al.hold_bitmap_drawing(true)
    al.draw_text(ex.f3, black, target_w, 0, string.format('%.1f FPS', ex.fps), {right = true})
    al.hold_bitmap_drawing(false)
end

local display = assert(al.create_display(640, 480))
al.destroy_bitmap(al.get_target_bitmap())

-- load fonts
ex.f1 = assert(al.load_font(font_file, 48, {}))
ex.f2 = assert(al.load_font(font_file, 48, {no_kerning = true}))
ex.f3 = assert(al.load_font(font_file, 12, {}))
ex.f4 = assert(al.load_font(font_file, -120, {}))
ex.f5 = assert(al.load_font(font_file, 12, {monochrome = true}))
-- config
ex.config = assert(al.load_config_file("/data/ex_ttf.ini"))
local timer = al.create_timer(1.0 / 60.0)
local queue = al.create_event_queue()
al.register_event_source(queue, al.create_keyboard_state())
al.register_event_source(queue, display)
al.register_event_source(queue, timer)

al.start_timer(timer)
local redraw = 0
local t = 0
ex.fps = 0
while true do
    local event = al.wait_for_event(queue)
    if event.type == 'display_close' then
        return
    elseif event.type == 'key_down' then
        if event.keycode == 59 then
            return
        else
            print(collectgarbage('count')) -- test
        end
    elseif event.type == 'timer' then
        redraw = redraw + 1
    end
    if redraw > 0 and al.is_event_queue_empty(queue) then
        redraw = redraw - 1
        local dt = al.get_time()
        render()
        dt = al.get_time() - dt
        
        t = 0.99 * t + 0.01 * dt

        ex.fps = 1.0 / t
        al.flip_display()
    end
end
