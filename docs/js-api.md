# Справочник JavaScript API

MyOS исполняет собственное подмножество JavaScript, а не browser/Node.js.
Системный модуль подключается явно:

```js
const myos = require("myos")
```

Системные методы ничего не печатают. Единственный обычный способ явного
вывода JS — `console.log(value)`; оконный текст выводится через методы Window.

## Типы и объекты

| JS-тип | Представление | Пример |
|---|---|---|
| number | знаковое целое `long` | `const pid = myos.getPid()` |
| boolean | числовое true/false runtime | `const ready = pid > 0` |
| string | динамическая heap-строка | `const text = myos.readFile("NOTES.TXT")` |
| object | список именованных свойств | `const p = {x: 10, title: "A"}` |
| array | до 32 смешанных элементов | `const a = [1, "x", {ok: 1}]` |
| module | handle встроенного `myos` | `require("myos")` |
| Window | handle терминального GUI-окна | `myos.createWindow()` |
| Screen | handle retained-слоя | `myos.createScreen()` |
| ProcessInfo[] | встроенный массив процессов | `myos.listProcesses()` |

Строки, объекты и массивы копируются при передаче функции и освобождаются при
выходе из локальной области. `null`, методы объектов, prototype и exceptions
пока не реализованы.

## `console`

### `console.log(value)`

- Вход: число, строка, object или array.
- Выход: отсутствует.
- Эффект: рекурсивно форматирует значение и пишет строку в console/terminal
  channel процесса.

```js
const value = {pid: myos.getPid(), tags: ["gui", "js"]}
console.log(value)
```

## Методы `myos`

### Файлы

| Сигнатура | Вход | Выход |
|---|---|---|
| `readFile(name)` | строка FAT 8.3 (`"NOTES.TXT"`) | string с байтами файла; числовой отрицательный код при ошибке |
| `writeFile(name, value)` | имя и string | number: размер или код ошибки |
| `listFiles()` | нет | string со списком root directory |

```js
const oldText = myos.readFile("NOTES.TXT")
const result = myos.writeFile("COPY.TXT", oldText)
console.log(result)
```

### Процессы

| Сигнатура | Вход | Выход/эффект |
|---|---|---|
| `getPid()` | нет | number PID, ничего не печатает |
| `getAppType()` | нет | `0` console, `1` GUI |
| `exec(command)` | string | заменяет текущую программу; number ошибки |
| `run(command)` | string | запускает console child с ожиданием |
| `spawn(command)` | string | PID console child либо ошибка |
| `spawnGui(command)` | string | PID GUI child либо ошибка |
| `kill(pid)` | number PID | завершает процесс, результата нет |
| `exit()` | нет | завершает текущий child, не возвращается |
| `listProcesses()` | нет | `ProcessInfo[]`, максимум 8 записей |

`ProcessInfo` предоставляет поля:

| Поле | Тип | Значение |
|---|---|---|
| `pid` | number | идентификатор |
| `name` | string | имя image до 8 символов |
| `active` | number | внутренний runnable-флаг |
| `state` | string | вычисленное текстовое состояние |
| `type` | string | текстовый тип приложения |
| `appType` | number | `0` или `1` |
| `parentPid` | number | PID родителя |
| `terminalPid` | number | terminal channel |

```js
const processes = myos.listProcesses()
for (let i = 0; i < processes.length; i++) {
    console.log(processes[i])
}
```

### Время и планирование

| Сигнатура | Вход | Выход/эффект |
|---|---|---|
| `millis()` | нет | number: монотонные миллисекунды PIT |
| `yield()` | нет | добровольно отдаёт остаток кванта |
| `idle()` | нет | бесконечная функция; процесс всё равно вытесняется PIT |

`yield()` не нужен для корректной работы других процессов.

### Клавиатура, события и terminal channel

| Сигнатура | Вход | Выход/эффект |
|---|---|---|
| `readKey()` | нет | number клавиши или `0` |
| `pollKeyEvent()` | нет | number только следующего key event либо `0` |
| `pollEvent()` | нет | объект `GuiEvent` |
| `sendKey(pid, key)` | два number | доставляет байт процессу |
| `terminalRead(pid)` | PID | string новых байтов terminal channel |

Объект `GuiEvent` всегда имеет поля `type`, `x`, `y`, `value`:

- `type == 0`: очередь пуста;
- `type == 1`: клавиша, код лежит в `value`;
- `type == 2`: движение, координаты в `x/y`, состояние ЛКМ в `value`;
- `type == 3`: кнопка; `1` press, `0` release, `2` cancel.

```js
const event = myos.pollEvent()
if (event.type == 1) {
    myos.sendKey(activePid, event.value)
}
```

### GUI и питание

| Сигнатура | Вход | Выход |
|---|---|---|
| `createWindow()` | нет | Window handle либо отрицательное число |
| `createScreen()` | нет | Screen handle для GUI-процесса |
| `nextWindow()` | нет | PID следующего запроса либо `-1` |
| `getWindowTitle(pid)` | PID | string заголовка либо `0` |
| `poweroff()` | нет | выключает систему, не возвращается |
| `clear()` | нет | очищает текущий console/terminal channel |

## Объект `Window`

| Метод | Вход | Выход/эффект |
|---|---|---|
| `setTitle(text)` | string ≤31 символа | публикует заголовок процесса |
| `clear()` | нет | очищает terminal snapshot |
| `beginUpdate()` | нет | начинает скрытый пакетный кадр |
| `endUpdate()` | нет | атомарно публикует кадр |
| `write(value)` | string | добавляет текст |
| `writeCell(value, width)` | string/number и ширина | значение и padding до ширины |
| `wait()` | нет | бесконечное ожидание приложения |

```js
const window = myos.createWindow()
window.setTitle("PROCESS MONITOR")
window.beginUpdate()
window.writeCell("PID", 6)
window.writeCell("NAME", 12)
window.write("\n")
window.endUpdate()
window.wait()
```

## Объект `Screen`

Screen формирует retained-команды. Изменения видимы только после `present()`.

| Метод | Вход | Результат |
|---|---|---|
| `setLayer(layer)` | `0..2` | меняет слой следующих команд |
| `fillRect(x,y,w,h,color)` | пять number, цвет `0xRRGGBB` | прямоугольник |
| `drawText(x,y,text)` | координаты и string | текст |
| `drawTerminal(pid,x,y,w,h)` | PID и прямоугольник | terminal snapshot |
| `drawImage(name,x,y)` | FAT 8.3 PNG и координаты | изображение |
| `drawCursor(x,y)` | координаты | атомарный cursor primitive |
| `focus(pid)` | PID | направляет клавиатуру процессу |
| `present()` | нет | фиксирует retained-кадр |

Слой `0` находится под окнами, `1` — над окнами, `2` — верхний UI/курсор.
На стороне kernel очередь имеет 16 команд. Runtime повторяет submit ограниченное
число раз; backend обрабатывает очередь порциями и объединяет `present`, чтобы
не блокировать ввод.

```js
const screen = myos.createScreen()
screen.setLayer(1)
screen.fillRect(20, 20, 240, 48, 2105376)
screen.drawText(32, 34, "HELLO")
screen.present()
```

## Таймеры

| Функция | Вход | Выход |
|---|---|---|
| `setTimeout(callback, delay)` | имя функции, ms ≥0 | ID `1..8` |
| `setInterval(callback, delay)` | имя функции, ms ≥0 | ID `1..8` |
| `clearTimeout(id)` | ID | `0`; неизвестный ID игнорируется |
| `clearInterval(id)` | ID | `0`; неизвестный ID игнорируется |

```js
const tick = () => {
    console.log(myos.millis())
}
const id = setInterval(tick, 1000)

const stop = () => {
    clearInterval(id)
}
setTimeout(stop, 5000)
```

Runtime блокирует процесс через `sys_wait_until` до ближайшего deadline; это
не busy-loop. Callback не прерывает выполняющийся JS-код и запускается после
возврата event loop к таблице таймеров.

## Пользовательские модули и функции

`require("file.js")` загружает файл до основного скрипта. Это текстовое
объединение, а не CommonJS: `exports`, кэш модулей и относительные каталоги
отсутствуют.

```js
// math.js
function add(a, b) {
    return a + b
}

// main.js
require("math.js")
console.log(add(20, 22))
```

Поддерживаются обычные и блочные стрелочные функции, до 8 параметров. Общие
лимиты runtime: source 12288 байт, 512 строк, строка 256 байт, 128 переменных,
16 функций, 8 таймеров, глубина блоков 12, массив 32 элемента.

## Не поддерживается

- browser DOM, Node.js, Promise/async/await;
- floating point, Date, JSON и стандартные коллекции;
- `null`, exceptions, classes, prototype и object methods;
- многострочное выражение произвольной формы;
- полноценные CommonJS modules;
- автоматическое ожидание GUI-события без polling.
