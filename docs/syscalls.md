# Системные вызовы и C API

Этот документ описывает фактически реализованный ABI. Публичные объявления
находятся в `pm32/include/myos/api.h` и `pm32/include/myos/gui.h`, структуры —
в `pm32/include/myos/types.h`.

## ABI `int 80h`

Пользовательская программа передаёт номер вызова в `EAX`, первый аргумент в
`EBX`, второй — в `ESI`. Результат возвращается в `EAX`. Переход в CPL0
выполняется инструкцией `int 80h`, возврат — через `IRETD`.

ASM-мост `pm32/kernel/syscalls.asm` выбирает supervisor-only `KernelRequest`
по текущему `CR3`, сохраняет пользовательские регистры и вызывает C-функцию
`kernel_dispatch`. Отдельный trap frame каждого процесса позволяет
диспетчеру переключить `CR3`, `EIP` и `ESP` прямо во время syscall. IRQ0 имеет
отдельный внутренний frame и не перезаписывает syscall другого процесса.

Общий формат вызова:

```c
/* pragma-обёртка загружает регистры автоматически */
int pid = sys_get_pid();
```

Необработанный номер возвращает `(u32)-1`. Указатели пока не проходят полную
проверку диапазона пользовательской памяти — это отмечено в `TODO.md`.

## Структуры данных

### `FileRequest`

| Поле | Тип | Вход/выход | Значение |
|---|---|---|---|
| `name` | `const char *` | вход | ровно 11 байт FAT 8.3, например `"NOTES   TXT"` |
| `destination` | `void *` | выход | буфер получателя |
| `capacity` | `u32` | вход | доступный размер буфера |

### `WriteRequest`

| Поле | Тип | Вход/выход | Значение |
|---|---|---|---|
| `name` | `const char *` | вход | имя FAT 8.3 длиной 11 байт |
| `source` | `const void *` | вход | записываемые байты |
| `size` | `u32` | вход | число байтов |

### `ProcessInfo`

`pid`, `active`, `app_type`, `parent_pid`, `terminal_pid` имеют тип `u32`;
`name` — строка до восьми символов плюс `\0`. `app_type` равен
`APP_CONSOLE` (`0`) или `APP_GUI` (`1`).

### GUI-команды и события

`GuiDrawCommand` содержит `type`, PID источника/цели, `layer`, координаты,
размер, цвет и строку до 63 символов. Типы: `FILL_RECT`, `TEXT`, `PRESENT`,
`TERMINAL`, `IMAGE`, `FOCUS`, `CURSOR` (номера 1–7).

`GuiEvent` содержит `type`, `x`, `y`, `value`. Типы событий: клавиша (`1`),
движение мыши (`2`), кнопка мыши (`3`). Для кнопки используются release `0`,
press `1`, cancel `2`.

## Публичные системные вызовы

| № | C-функция | Вход | Выход |
|---:|---|---|---|
| 1 | `sys_write(text, length)` | указатель и длина | число записанных байтов/код |
| 2 | `sys_exit()` | нет | не возвращается при успешном завершении |
| 3 | `sys_exec(command)` | строка команды | `0` при успешной замене, отрицательный код при ошибке |
| 4 | `sys_read_key()` | нет | символ/scan-код, `0` если данных нет |
| 5 | `sys_read_file(request)` | `FileRequest *` | размер файла или отрицательный код |
| 6 | `sys_spawn(name)` | команда/имя | PID либо отрицательный код |
| 7 | `sys_yield()` | нет | нет; добровольно отдаёт остаток кванта |
| 8 | `sys_send_key(pid, key)` | PID, байт клавиши | нет |
| 9 | `sys_terminal_read(pid, buffer)` | PID и буфер ≥256 байт | число новых байтов |
| 10 | `sys_get_pid()` | нет | числовой PID |
| 11 | `sys_kill(pid)` | PID | нет; разрешено GUI-серверу PID 0 |
| 14 | `sys_run(command)` | команда | результат запуска после ожидания child |
| 15 | `sys_write_file(request)` | `WriteRequest *` | размер/код ошибки |
| 16 | `sys_get_app_type()` | нет | `APP_CONSOLE` или `APP_GUI` |
| 17 | `sys_spawn_gui(command)` | строка команды | PID GUI-процесса либо ошибка |
| 18 | `sys_list_files(buffer, capacity)` | выходной буфер | длина текста списка либо ошибка |
| 19 | `sys_poweroff()` | нет | не возвращается |
| 20 | `sys_gui_create_window()` | нет | `0` либо ошибка доступа |
| 21 | `sys_gui_next_window()` | нет | PID заявителя либо `-1` |
| 22 | `sys_list_processes(array, capacity)` | массив и число записей | фактическое число `ProcessInfo` |
| 23 | `sys_gui_set_title(title)` | строка ≤31 символа | сохранённая длина либо ошибка |
| 24 | `sys_gui_get_title(pid, title)` | PID, буфер ≥32 байт | длина либо ошибка |
| 25 | `sys_malloc(size)` | размер >0 | обнулённый указатель либо `NULL` |
| 26 | `sys_free(pointer)` | ранее выданный указатель | `0` либо ошибка |
| 27 | `sys_millis()` | нет | монотонные миллисекунды PIT (`u32`) |
| 28 | `sys_gui_submit_draw(command)` | `GuiDrawCommand *` | `0`, `-1` при заполненной очереди/запрете |
| 29 | `sys_gui_next_draw(command)` | выходная структура | `0` при команде, `-1` если пусто |
| 30 | `sys_gui_poll_event(event)` | выходная структура | `1` при событии, `0` если пусто |
| 31 | `sys_gui_send_event(pid, event)` | PID и событие | `0` либо `-1` |
| 32 | `sys_terminal_snapshot(pid, buffer)` | PID и буфер ≥4096 байт | размер snapshot |
| 33 | `sys_gui_set_focus(pid)` | PID | `0` либо `-1` |
| 34 | `sys_gui_get_focus()` | нет | PID в фокусе |
| 36 | `sys_wait_until(deadline)` | абсолютное значение `sys_millis()` | после достижения deadline |
| 37 | `sys_input_read()` | нет; только GUI-сервер | клавиатурный байт, mouse packet или `0` |

Номера 12 и 13 используются только при загрузке PID 0. Номер 35 — внутренний
запрос планировщика от IRQ0; пользовательская программа не должна его вызывать.

## Примеры C

Чтение файла:

```c
char data[4097];
FileRequest request = { "NOTES   TXT", data, 4096 };
int size = sys_read_file(&request);
if (size >= 0) data[size] = 0;
```

Динамическая память:

```c
char *buffer = (char *)sys_malloc(1024);
if (buffer) {
    /* работа с buffer */
    sys_free(buffer);
}
```

Список процессов:

```c
ProcessInfo processes[8];
int count = sys_list_processes(processes, 8);
```

Ожидание без busy-loop:

```c
unsigned deadline = sys_millis() + 5000;
sys_wait_until(deadline);
```

## Подсистемы диспетчера ядра

- `dispatch.c` — процессы, файлы и общая маршрутизация;
- `syscall_input.c` — объединённый поток клавиатуры/мыши для GUI-сервера;
- `syscall_time.c` — часы, yield и блокирующее ожидание;
- `syscall_memory.c` — процессный heap;
- `syscall_gui_ipc.c` — draw/event очереди, terminal snapshot и focus.

При добавлении syscall нужно синхронно изменить ASM/C-обёртку, dispatcher,
структуры ABI при необходимости и этот документ.
