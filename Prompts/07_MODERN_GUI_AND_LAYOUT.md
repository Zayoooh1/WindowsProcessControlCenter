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


# Task 07 — Nowoczesne GUI i dopracowany layout

## Cel

Dopracuj UI tak, żeby aplikacja wyglądała jak przyjemne narzędzie z 2026 roku, a nie klasyczny program z Windows XP.

## Priorytet

To jest etap polishu UI. Nie dodawaj nowych ciężkich funkcji systemowych, jeśli obecne nie są stabilne.

## Layout docelowy

Okno główne:

```text
┌─────────────────────────────────────────────────────────────┐
│ Top bar: nazwa aplikacji, status admina, refresh, settings  │
├─────────────────────────────────────────────────────────────┤
│ Filters: search, user/system toggle, auto refresh, sort     │
├───────────────────────────────────────┬─────────────────────┤
│ Process table                         │ Details panel       │
│ - Name                                │ - Name/PID/path     │
│ - PID                                 │ - CPU priority      │
│ - RAM                                 │ - Freeze/Resume     │
│ - Priority                            │ - GPU preference    │
│ - Status                              │ - Safe actions      │
├───────────────────────────────────────┴─────────────────────┤
│ Operation log                                                │
└─────────────────────────────────────────────────────────────┘
```

## Wymagania wizualne

- Ciemny motyw.
- Spójne odstępy.
- Maksymalnie 2-3 akcenty kolorystyczne.
- Czytelne fonty.
- Wyraźne stany: disabled, warning, danger.
- Żadnego nachodzenia elementów.
- Żadnej tabeli rozciągniętej bez sensu.
- Niech prawy panel ma stałą minimalną szerokość.
- Dolny log ma mieć ograniczoną wysokość i przewijanie.
- Przy małym oknie pokaż scroll, zamiast ściskać wszystko na siłę.

## UX

Dodaj:

- global search,
- filtry: All / User processes / System-like / Inaccessible,
- checkbox Auto refresh,
- status bar: liczba procesów, ostatnie odświeżenie, tryb admin/user,
- tooltipy do ryzykownych akcji,
- empty state, gdy filtr nic nie pokazuje,
- ikonki tekstowe lub proste symbole, ale bez przesady.

## Dostępność

- UI ma działać przy DPI 100%, 125%, 150%.
- Kontrast tekstu ma być dobry.
- Nie rób mikroskopijnych przycisków.
- Każda ważna akcja ma mieć opis albo tooltip.

## Kryteria ukończenia

- UI jest czytelne na 1280x800.
- Nic na siebie nie nachodzi.
- Procesy da się wygodnie filtrować i sortować.
- Prawy panel nie jest przeładowany.
- Ryzykowne akcje wyglądają wyraźnie inaczej od zwykłych.
- `docs/PROGRESS.md` zawiera krótki opis zmian UI.

## Build i test

Uruchom build Debug i Release.  
Sprawdź ręcznie DPI 100%, 125%, 150%, jeśli środowisko pozwala.

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
