# MicPlus

Глобальная система контроля жизни микрофона: **вкл/выкл/рация**.

Проект состоит из двух утилит:

- `micplus.exe` — трэй-приложение с иконкой, PTT-режимом и глобальными хоткеями.
- `micctl.exe` — консольная утилита для мгновенного включения/выключения микрофона.

> English description is available below.

---

## Возможности

- Режим рации **push-to-talk (PTT)**:
  - боковые кнопки мыши (MB4/MB5);
  - любая клавиша клавиатуры/мыши.
- Глобальное переключение **toggle-хоткей**:
  - F9 / F10 из коробки;
  - любое пользовательское сочетание (клавиша либо Ctrl/Alt/Shift/Win + клавиша).
- Индикатор состояния микрофона в трее:
  - отдельные иконки для включённого и выключенного микрофона.
- Сохранение настроек в `micplus.ini` (горячие клавиши, язык, PTT-режим).
- Два языка интерфейса: **русский / английский**.

## Установка

1. Зайдите в раздел [Releases](https://github.com/nivemoon/micplus/releases) и скачайте последний релиз (`micplus.exe` и `micctl.exe`).
2. Поместите оба файла в одну папку.
3. Запустите `micplus.exe`.
4. В системном трее появится иконка MicPlus.

Файл `micplus.ini` будет создан автоматически рядом с `micplus.exe`.

## Использование

- Правый клик по иконке в трее:
  - выбор PTT-кнопки (MB4 / MB5 или кастомная клавиша);
  - выбор toggle-хоткея (F9 / F10 / Назначить);
  - включение/выключение PTT-режима;
  - смена языка интерфейса;
  - выход из приложения.
- При зажатой PTT-кнопке микрофон включается, при отпускании — выключается.
- Toggle-хоткей принудительно включает или выключает микрофон вне зависимости от PTT.

## Сборка из исходников

Исходники и ресурсы лежат в каталогах `src/` и `res/`.

Требуется MinGW-w64 или другой компилятор с поддержкой WinAPI.

Пример сборки (из корня репозитория):

```bash
windres res/micplus.rc -O coff -o micplus.res
gcc src/micplus.c micplus.res -o micplus.exe -mwindows -lole32 -luuid -luser32 -lshell32
```

`micctl.exe` собирается отдельно из `src/micctl.c` аналогичным образом.

## Структура репозитория

```text
src/        # исходники (micplus.c, micctl.c)
res/        # ресурсы (micplus.rc, .ico)
README.md
LICENSE
```

Готовые бинарники (`micplus.exe`, `micctl.exe`) публикуются в разделе Releases и в репозитории по умолчанию не хранятся.

## Лицензия

Проект распространяется по лицензии MIT. См. файл [`LICENSE`](LICENSE).

---

## English summary

**MicPlus** is a small Windows tray utility that gives you global control over your microphone: **mute / unmute / push-to-talk (PTT)**.

It consists of two parts:

- `micplus.exe` — tray app with an icon, PTT mode and global hotkeys.
- `micctl.exe` — console helper that directly toggles the microphone state.

### Features

- **Push-to-talk (PTT)**:
  - mouse side buttons (MB4 / MB5);
  - any keyboard key.
- **Toggle hotkey**:
  - F9 / F10 by default;
  - any custom combination (key or Ctrl/Alt/Shift/Win + key).
- Tray icon indicates microphone state:
  - separate icons for mic on / mic off.
- Settings saved to `micplus.ini` (hotkeys, language, PTT mode).
- UI languages: **Russian / English**.

### Installation

1. Download the latest release from [Releases](https://github.com/nivemoon/micplus/releases).
2. Put `micplus.exe` and `micctl.exe` into the same folder.
3. Run `micplus.exe` — a MicPlus icon will appear in the system tray.
4. Configure hotkeys and PTT mode from the tray context menu.

### Build

See the Russian section above for a MinGW / gcc build example from `src/` and `res/` folders.
