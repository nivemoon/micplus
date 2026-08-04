# MicPlus

Портативная система контроля микрофона: **вкл/выкл/ртт**.

Проект состоит из двух утилит и папки со звуками:

- `micplus.exe` — трэй-приложение с иконкой, PTT-режимом и глобальными хоткеями.
- `micctl.exe` — консольная утилита мгновенного переключания микрофона.
- `sounds` - папка с аудиосигналами (можете заменить своими)
<img width="230" height="124" alt="ejDg8Sypqa" src="https://github.com/user-attachments/assets/7494c5d3-9372-4282-b74f-9d30aa2982f8" />
<img width="218" height="95" alt="ElUs88mR2L" src="https://github.com/user-attachments/assets/004a302d-aca5-47e5-8b86-e8757774e923" />

> English description is available below.

# Обновление 1.4

- Отныне утилита использует низкоуровневые хуки. Говоря проще, ускорена скорость срабатывания и добавлена поддержка туда, где имелись проблемы (League of Legends, PUBG, Throne and Liberty и т.д). 
- Перестроен функционал выбора и назначения клавиш. На данный момент, вы даже можете выбрать одну клавишу для обоих режимов (но смысла в этом не будет). Кнопки мыши допустимы только для режима РТТ. 
- Исправлены системные уведомления и добавление плашки "о программе", с отображением текущей версии.
- Управление звуком..упразднено. Существует лишь конкретная комбинация Ctrl + F9 для переключения звука в системе.
- Малое исправление: более чистое отображение в диспетчере задач. Иконка в диспетчере меняетя вместе иконкой на панели задач (незапланированный эффект).

# Обновление 1.33_afix

- Изменён механизм автозагрузки с системой, в связи с ложной индикацией функционала (спасибо, https://github.com/Vektor010). Говоря проще, утилита при автозапуске с системой лишь делала вид, что умеет переключать микрофон, всегда оставляя его включённым (если не включен режим ртт). С этих пор, система использует безопасный метод создания/удаления ярлыка в системную папку автозагрузок, вместо реестра. 
(%userprofile%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup)

# Обновление 1.3

- Переписаны названия меню, строк и прочее прочее.
- Добавлено: автозапуск с системой, звуковые индикаторы, системные уведомления
- Новые иконки приложения: чёрная, белая, красная (при отключенном микрофоне):
      - По умолчанию, программа сама определит текущий цветовой режим и подберёт цвет. Имеется так же ручное переключение в настройках.

---

## Возможности

- Режим рации (PTT):
    - Выбор из боковык кнопок мыши и тильды. Назначение: любая клавиша клавиатуры/мыши. По умолчанию выбрана боковая нижняя кнопка мыши (MB5).
- Режим микрофона и звук в системе:
    - Выбор из F9, F10 и тильды. Назначение: любая клавиша клавиатуры или сочетание с модификатором Ctrl/Alt/Shift/Win. По умолчанию выбрана F10.
- Отдельная скрытая функция: переключение звука в системе на Ctrl+F9;
  _примечание_. Из-за особенностей Windows и русской раскладки, правый Alt обладает непредсказуемостью и может назначиться как Ctrl+Alt. 
- Индикатор состояния микрофона в трее (опционально включаются сигналы на оба режима микрофона и системные уведомления):
  - отдельные иконки для включённого и выключенного микрофона.
- Сохранение настроек в `micplus.ini` (горячие клавиши, язык, PTT-режим).
- Два языка интерфейса: **русский / английский**.

## Установка

1. В разделе [Releases](https://github.com/nivemoon/micplus/releases) и скачать последний релиз в архиве zip. (`micplus.exe`, `micctl.exe`, sounds).
2. Поместите оба файла в одну папку.
3. Запустите `micplus.exe`.
4. В системном трее появится иконка MicPlus.

Файл `micplus.ini` будет создан автоматически рядом с `micplus.exe`.

## Использование

- Правый клик по иконке в трее:
  - Включение режима PTT (MB5 или кастомная клавиша);
  - Включение микрофона (F10 / своё);
  - сочетания клавиш
  - настройки
  - 
  *Режим РТТ (push-to-talk или же рация) - при зажатой кнопке микрофон включается, при отпускании — выключается.
  *Включение и отключение микрофона работают вне завсимости от режима РТТ. Включение режима РТТ автоматически выключит микрофон. 

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
