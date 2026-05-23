# Wspólne założenia projektu

Projekt: **Windows Process Control Center**  
Język: **C++20**  
Platforma: **Windows 10/11 x64**  
Build: **CMake + MSVC / Visual Studio 2022**  
GUI: **Win32 + DirectX 11 + Dear ImGui**, bez klasycznych kontrolek Win32 wyglądających jak Windows XP.  
Styl UI: nowoczesny, ciemny, spokojny, czytelny, bez przeładowania elementami.

## Najważniejsze zasady

- To ma być normalne narzędzie administracyjne, nie malware.
- Nie dodawaj ukrywania procesów, injection, keyloggerów, driverów kernela, omijania zabezpieczeń ani funkcji stealth.
- Operacje destrukcyjne, takie jak zakończenie procesu lub priorytet czasu rzeczywistego, wymagają czytelnego ostrzeżenia.
- Aplikacja nie może „udawać”, że zrobiła coś, czego Windows realnie nie pozwala zrobić.
- Preferencja GPU ma być pokazana uczciwie: Windows zwykle ustawia ją per ścieżka `.exe`, a efekt może wymagać restartu aplikacji/procesu.
- Dla procesów systemowych, chronionych, CSRSS, Idle, services hostów i własnego procesu aplikacji akcje ryzykowne mają być domyślnie zablokowane albo wymagać bardzo wyraźnego potwierdzenia.
- Każdy etap musi budować się poprawnie.
- Po każdej większej zmianie uruchom build i zostaw krótką notatkę w `docs/PROGRESS.md`.

## Minimalna struktura repozytorium

```text
WindowsProcessControlCenter/
  CMakeLists.txt
  README.md
  docs/
    ARCHITECTURE.md
    PROGRESS.md
    SAFETY_NOTES.md
  src/
    main.cpp
    app/
      Application.h
      Application.cpp
    core/
      ProcessInfo.h
      ProcessEnumerator.h
      ProcessEnumerator.cpp
      ProcessActions.h
      ProcessActions.cpp
      PriorityManager.h
      PriorityManager.cpp
      SuspensionManager.h
      SuspensionManager.cpp
      GpuPreferenceManager.h
      GpuPreferenceManager.cpp
      PrivilegeHelper.h
      PrivilegeHelper.cpp
      ErrorUtils.h
      ErrorUtils.cpp
    ui/
      UiShell.h
      UiShell.cpp
      ProcessTableView.h
      ProcessTableView.cpp
      Theme.h
      Theme.cpp
    platform/
      WinHandles.h
      WinHandles.cpp
  third_party/
    imgui/
```


# Task 09 — Ustawienia, logi i zapamiętywanie preferencji

## Cel

Dodaj lekką warstwę ustawień aplikacji i czytelny log operacji.

## Zakres

Dodaj lub rozszerz:

```text
src/core/Settings.h
src/core/Settings.cpp
src/core/OperationLog.h
src/core/OperationLog.cpp
src/ui/SettingsPanel.h
src/ui/SettingsPanel.cpp
```

## Ustawienia do zapamiętania

Zapisuj lokalnie w pliku JSON w profilu użytkownika, np.:

```text
%APPDATA%\WindowsProcessControlCenter\settings.json
```

Zapamiętaj:

- rozmiar okna,
- auto-refresh on/off,
- interwał auto-refresh,
- ostatnie filtry,
- ostatni sort,
- czy log ma automatycznie przewijać na dół,
- motyw: dark / darker / high contrast,
- czy pokazywać procesy systemowe.

## Log operacji

Log ma zawierać:

- timestamp,
- typ akcji,
- PID,
- nazwa procesu,
- wynik: success / warning / failed / blocked,
- krótki komunikat.

Dodaj możliwość:

- copy log,
- clear log,
- export log to `.txt`.

## Zasady prywatności

- Nie zapisuj pełnej listy procesów automatycznie do pliku.
- Nie wysyłaj żadnych danych do internetu.
- Nie dodawaj telemetrii.
- Log eksportowany tylko ręcznie przez użytkownika.

## UI

Dodaj prosty panel ustawień, najlepiej modal albo prawy drawer:

- Appearance,
- Refresh,
- Safety,
- Logs.

Nie rób przeładowanego panelu. Ma być krótko i czytelnie.

## Kryteria ukończenia

- Ustawienia zapisują się i odczytują po restarcie aplikacji.
- Log pokazuje wszystkie akcje.
- Export log działa.
- Aplikacja działa bez internetu i bez telemetrii.
- Uszkodzony `settings.json` nie crashuje aplikacji — reset do domyślnych.

## Build i test

Uruchom build Debug i Release.  
Przetestuj zmianę ustawień, restart aplikacji i eksport logu.

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
