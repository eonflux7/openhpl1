#!/usr/bin/env python3
# tools/porting/disassemble_game.py bp "cMainMenu::CreateWidgets" > cw.txt; summarize_menu_widgets.py cw.txt
# Summarise cMainMenu::CreateWidgets: widget class, strings/floats seen since the
# previous widget, and the state list it is added to (0x6c + 8*state).
import re, sys
lines = open(sys.argv[1]).read().splitlines()
pending_values, current_widget = [], None
widgets = []
for line_index, line in enumerate(lines):
    m = re.search(r'; (L?".*")$', line)
    if m and 'Translate' not in line:
        pending_values.append(m.group(1))
    m = re.search(r'movl\s+\$0x(4[0-4][0-9a-f]{6}|c[0-4][0-9a-f]{6}),', line)
    if m:
        import struct
        pending_values.append('%g' % struct.unpack('<f', bytes.fromhex(m.group(1))[::-1])[0])
    m = re.search(r'call\s+\w+ <(cMainMenuWidget_\w+)::cMainMenuWidget_\w+\(', line)
    if m:
        current_widget = [m.group(1), pending_values]; pending_values = []
        widgets.append(current_widget)
    m = re.search(r'add\s+\$0x([0-9a-f]+),%e[a-z]{2}$', line)
    if m and current_widget is not None and 'state' not in current_widget:
        # only counts if a 0x6c load came just before
        if any('0x6c(' in x for x in lines[line_index-3:line_index]):
            current_widget.append('state'); current_widget.append(int(m.group(1), 16)//8)
    if re.search(r'mov\s+0x6c\(%e..\),%e..$', line) and current_widget is not None and 'state' not in current_widget:
        # state 0 has no add
        next_lines = lines[line_index+1:line_index+4]
        if not any(re.search(r'add\s+\$0x', x) for x in next_lines):
            current_widget.append('state'); current_widget.append(0)
for widget in widgets:
    state = widget[3] if len(widget) > 3 else '?'
    print('%-4s %-34s %s' % (state, widget[0], ' '.join(widget[1])[:150]))
