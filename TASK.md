# TASK — реестр AllToZed

_Окружение: W = 200K, профиль: стандарт_
_Рабочий язык: русский_
_Триггеры: «продолжаем» / «продолжи» / «далее» — дополняй или исключай на этой строке (протокол AGENTS.md следует ей)_

## Активная задача

### Задача «Кнопка SEND в окне Edit Macro» [EXECUTED]
- **Цель:** В окне Edit Macro (режим MODE: EDIT) — кнопка SEND слева, ниже двух строк Name/CMD; отправляет текущее содержимое строки CMD в порт (без сохранения в банк, без закрытия окна).
- **Артефакт:** `main.c` — новый `IDC_BTN_MACRO_SEND 3005`, поле `hBtnSend` в `MacroEditCtx`, создание/раскладка кнопки, функция `SendMacroTextRaw`, обработка `WM_COMMAND`.
- **Как:**
  1. `#define IDC_BTN_MACRO_SEND 3005` (после 3004).
  2. `MacroEditCtx`: `HWND hBtnSend`.
  3. Ядро отправки из `SendMacroCommand` вынести в `SendMacroTextRaw(const wchar_t*, HWND)`; `SendMacroCommand` — тонкая обёртка (индексы + ядро). Ядро: проверка порта/пустоты, `parse_text_with_inline_hex`, запись + echo.
  4. `MacroEditWndProc` WM_CREATE: создать кнопку "Send"; `LayoutMacroEdit`: `x=10, y=btnY` (левая часть строки Save/Cancel, ниже двух строк).
  5. WM_COMMAND: `IDC_BTN_MACRO_SEND` → `GetWindowTextW(hEditCmd)` → `SendMacroTextRaw(cmd, g_hwndMacroPads[bankIndex])`; окно **не** закрывается (Save/Cancel/Enter/Esc — как раньше).
- **Текущее состояние:**
  - сделано: `main.c` — шаги 1–5; сборка Pelles C успешна (`CC_EXIT=0`, `LINK_EXIT=0`, тестовый бинарник `output\WT232_send.exe`)
  - в работе: —
  - не начато: ручной runtime-тест (порт), подтверждение пользователя
- **Следующие 2–3 шага:**
  1. Запустить `output\WT232_send.exe`: панель макросов → R->E (MODE: EDIT) → клик по ячейке → в окне Edit Macro кнопка «Send» слева внизу; ввод в CMD → SEND → команда уходит в порт (эхо в терминале), окно остаётся открытым; порт не открыт → MessageBox.
  2. Пересобрать официальный `WT232.exe` в IDE Pelles C (.ppj), если тест пройден.
- **Решения:** | что | почему |
  | SEND не закрывает окно | итеративное тестирование команд; Save/Cancel — уже есть | 
  | вынести ядро `SendMacroTextRaw` | `SendMacroCommand` отправляет сохранённый слот, а кнопка — текущий текст контрола; общая логика записи/echo без дублей | 
  | SEND в строке Save/Cancel (btnY), слева | «ниже двух строк, слева»: совмещает с поведением LayoutMacroEdit при изменении ширины/высоты | 
- **Открытые вопросы:** —
- **Файлы для чтения:** `main.c` — `SendMacroCommand` (L1655), `MacroEditWndProc` (L1749), `LayoutMacroEdit` (L1645), `MacroEditCtx` (L330)
- **Проверка:** Pelles C `pocc -std:C11 -W1` → 0 ошибок; линковка `polink -machine:x86` → `output\WT232_send.exe`; ручной тест по шагу 1

## Реестр

`Исходное состояние проекта WT232 — AllToZed инициализирован, .gitignore под Pelles C — 2026-10-28`
`Сохранять размер окна макросов — реализовано W/H в MacroPos_%d, подтверждено пользователем — 2026-10-28`
`Редактор макроса: изменяемый по ширине + память размера/позиции — реализовано, только-ширина по фидбеку, подтверждено — 2026-10-28`
`v0.6.3 + README — версия/журнал/лицензия, архив v0.6.3.rar в репо, push; GitHub-релиз не сделан (решение пользователя) — 2026-10-28`
