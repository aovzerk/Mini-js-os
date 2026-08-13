# C-приложения MyOS

Пользовательские программы находятся в `pm32/user`, компилируются Open Watcom
в 32-битный код и упаковываются как плоские `.BIN`-файлы. Стандартной
библиотеки C нет: доступны syscall API MyOS и `userlib`.

## Точка входа

ОС начинает выполнение с первого байта `.BIN`, а не с entry point
LX-заголовка. `_start` должна быть первой определённой функцией исходного
файла. Перед ней допустимы include, define, данные и объявления функций.

```c
#include <myos/api.h>

static void program_main(void);

void _start(void)
{
    program_main();
}

static void program_main(void)
{
    sys_write("Hello from MyOS\n", 16);
    sys_exit();
}
```

Аргументы передаются в EBX. Для программы с аргументами используется:

```c
void _start(const char *arguments)
{
    program_main(arguments);
}

#pragma aux _start parm [ebx];
```

Определение helper-функции перед `_start` поместит её в начало плоского BIN.
Возврат из такой функции уйдёт на случайный адрес и без IDT может вызвать
triple fault.

## Консольная программа

Консольное приложение выводит данные через `sys_write`, читает клавиатуру через
`sys_read_key` и завершает работу через
`sys_exit`. `AppType = 0` разрешает запуск через `sys_spawn` и `sys_run`.

## GUI-программа

GUI-приложение имеет `AppType = 1`, запускается через `sys_spawn_gui` и само
запрашивает окно:

```c
#include <myos/api.h>
#include <myos/gui.h>

static void program_main(void);

void _start(void)
{
    program_main();
}

static void program_main(void)
{
    sys_gui_set_title("DEMO");
    if (sys_gui_create_window() < 0) sys_exit();

    sys_write("GUI application\n", 16);
    for (;;) sys_yield();
}
```

GUI-сервер получает запрос через `sys_gui_next_window`, заголовок — через
`sys_gui_get_title`. Приложение не изменяет массив окон GUI-сервера напрямую.

## Добавление в образ

1. Создать `pm32/user/name.c`.
2. Добавить `name` в `$programNames` файла `build32.ps1`.
3. Добавить FAT-запись в `$files` с именем ровно из 11 символов.
4. Указать `AppType = 0` для консоли или `AppType = 1` для GUI.
5. Собрать образ и проверить программу в QEMU.

```powershell
[pscustomobject]@{
    Name = "DEMO    BIN"
    Data = [IO.File]::ReadAllBytes("$buildDirectory\demo.bin")
    AppType = 1
},
```

Заголовки API находятся в `pm32/include/myos`, общая библиотека — в
`pm32/lib/userlib.c`. После изменения ядра или ABI журнал QEMU не должен
содержать `check_exception` и `Triple fault`.
