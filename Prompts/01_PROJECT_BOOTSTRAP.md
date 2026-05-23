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


# Task 01 — Bootstrap projektu od zera

## Cel

Stwórz nowy projekt C++ od podstaw jako aplikację desktopową Windows z nowoczesnym UI.

## Zakres

Zrób:

- repozytorium `WindowsProcessControlCenter`,
- `CMakeLists.txt`,
- build C++20 pod MSVC x64,
- bazową aplikację Win32,
- renderer DirectX 11,
- integrację Dear ImGui,
- ciemny motyw aplikacji,
- puste, stabilne okno główne,
- obsługę DPI scaling,
- podstawowy shell layoutu:
  - top bar,
  - panel filtrów,
  - główna tabela procesów jako placeholder,
  - prawy panel szczegółów jako placeholder,
  - dolny log operacji jako placeholder.

## Wymagania UI

- Zero klasycznego wyglądu Windows XP.
- Nie używaj surowych Win32 Button/ListView jako głównych kontrolek.
- UI ma być spokojne, ciemne, nowoczesne, z sensownymi odstępami.
- Wszystko ma się mieścić w oknie 1280x800.
- Layout nie może się rozjeżdżać przy DPI 125% i 150%.
- Nie dodawaj losowych bajerów, gradientów i przesadnych animacji.

## Wymagania techniczne

- Użyj RAII dla uchwytów WinAPI.
- Oddziel `app/`, `core/`, `ui/`, `platform/`.
- Nie dodawaj jeszcze realnej enumeracji procesów.
- Dodaj mockowe dane do tabeli, żeby sprawdzić layout.

## Pliki do utworzenia

```text
CMakeLists.txt
README.md
docs/ARCHITECTURE.md
docs/PROGRESS.md
docs/SAFETY_NOTES.md
src/main.cpp
src/app/Application.h
src/app/Application.cpp
src/ui/UiShell.h
src/ui/UiShell.cpp
src/ui/Theme.h
src/ui/Theme.cpp
src/platform/WinHandles.h
src/platform/WinHandles.cpp
```

## Kryteria ukończenia

- Projekt buduje się w trybie Debug i Release.
- Aplikacja otwiera okno bez crasha.
- UI jest responsywne, czytelne i nic na siebie nie nachodzi.
- W `docs/PROGRESS.md` wpisz, co zostało zrobione i jak uruchomić build.

## Build i test

Uruchom:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release
```

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
