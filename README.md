# MicPlus

Портативная система контроля микрофона: **вкл/выкл/ртт/звук**.

Проект состоит из двух утилит:

- `micplus.exe` — трэй-приложение с иконкой, PTT-режимом и глобальными хоткеями.
- `micctl.exe` — консольная утилита для мгновенного включения/выключения микрофона.

> English description is available below.

# Обновление 1.33_afix

- Изменён механизм автозагрузки с системой, в связи с ложной индикацией функционала (обранужено пользователем https://github.com/Vektor010). Говоря проще, утилита при автозапуске с системой лишь делала вид, что умеет переключаться микрофон. С этих пор, система использует безопасный метод создания/удаления ярлыка в %userprofile%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup, вместо реестра. 

# Обновление 1.3

- Переписаны названия меню, строк и прочее прочее.
- Добавлено: автозапуск с системой, звуковые индикаторы, системные уведомления
- Новые иконки приложения: чёрная, белая, красная (при отключенном микрофоне):
      - По умолчанию, программа сама определит текущий цветовой режим и подберёт цвет. Имеется так же ручное переключение в настройках.

---

## Возможности

- Режим рации (PTT):
    - любая клавиша клавиатуры/мыши. По умолчанию, боковая кнопка мыши (MB5).
- Режим микрофона и звук в системе:
  - F10 и Alt+F9 по умолчанию;
  - любое пользовательское сочетание (клавиша либо Ctrl/Alt/Shift/Win + клавиша).
*примечание. Из-за особенностей Windows и русской раскладки, правая клавиша Alt обладает непредсказуемостью и может назначится как Ctrl+Alt. 
- Индикатор состояния микрофона в трее (опционально вкчлаются сигналы на оба режима микрофона и системные уведомления):
  - отдельные иконки для включённого и выключенного микрофона.
- Сохранение настроек в `micplus.ini` (горячие клавиши, язык, PTT-режим).
- Два языка интерфейса: **русский / английский**.

## Установка

1. Зайдите в раздел [Releases](https://github.com/nivemoon/micplus/releases) и скачайте последний релиз (`micplus.exe`, `micctl.exe`, sounds).
2. Поместите оба файла в одну папку.
3. Запустите `micplus.exe`.
4. В системном трее появится иконка MicPlus.

Файл `micplus.ini` будет создан автоматически рядом с `micplus.exe`.

## Использование

- Правый клик по иконке в трее:
  - режим PTT-кнопки (MB5 или кастомная клавиша);
  - переключение микрофона (F10 / своё);
  - выбор сочетаний для ртт, включаетля микрофона, звука системы
  - автозапуск с системой, звки для режимов, системные уведомления
  - смена языка интерфейса;
  - выход из приложения.
- При зажатой PTT-кнопке микрофон включается, при отпускании — выключается.
- Toggle-хоткей принудительно включает или выключает микрофон вне зависимости от PTT.

## Сборка из исходников

Исходники и ресурсы лежат в каталогах `src/` и `res/`.

Требуется MinGW-w64 или другой компилятор с поддержкой WinAPI.

Пример сборки (из корня репозитория):

```bash
windres micplus.rc -O coff -o micplus.res
gcc micplus.c micplus.res -o micplus.exe -mwindows -luser32 -lshell32 -lwinmm -ladvapi32
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
  - mouse side button (MB5);
  - any keyboard key.
- **Toggle hotkey**:
  - F10 by default;
  - any custom combination (key or Ctrl/Alt/Shift/Win + key).
- **Sound key**
  - Alt+F9 by default;
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
