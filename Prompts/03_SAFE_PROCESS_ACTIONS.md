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


# Task 03 — Bezpieczne akcje na procesach

## Cel

Dodaj podstawowe, bezpieczne akcje administracyjne na wybranym procesie.

## Zakres

Dodaj moduł:

```text
src/core/ProcessActions.h
src/core/ProcessActions.cpp
src/core/PrivilegeHelper.h
src/core/PrivilegeHelper.cpp
```

## Akcje

Zaimplementuj:

1. **Terminate process**
   - użyj `OpenProcess(PROCESS_TERMINATE)` i `TerminateProcess`,
   - pokaż potwierdzenie przed zakończeniem,
   - po wywołaniu odśwież listę,
   - pokaż wynik w logu.

2. **Open file location**
   - otwórz Explorer z zaznaczonym plikiem `.exe`, jeśli ścieżka istnieje.

3. **Copy details**
   - skopiuj do schowka: nazwa, PID, ścieżka, priorytet, RAM, status.

4. **Refresh selected process**
   - odśwież dane konkretnego procesu.

## Blokady bezpieczeństwa

Nie pozwalaj przez zwykły klik zakończyć:

- własnego procesu aplikacji,
- PID 0 / Idle,
- PID 4 / System,
- CSRSS,
- Wininit,
- Services,
- LSASS,
- procesów oznaczonych jako krytyczne lub chronione,
- procesu bez pełnego potwierdzenia, jeśli jest uruchomiony jako administrator albo należy do innego użytkownika.

Dla procesów zablokowanych pokaż jasny komunikat: **dlaczego akcja jest niedostępna**.

## UX

- Przyciski akcji mają być w prawym panelu szczegółów.
- Akcje niedostępne mają być wyszarzone, nie ukryte.
- Każda akcja ma wpis w dolnym logu.
- Nie pokazuj ściany tekstu w popupach.

## Kryteria ukończenia

- Zakończenie działa na zwykłych aplikacjach użytkownika, np. Notepad.
- Aplikacja nie pozwala przypadkowo zabić samej siebie.
- Błędy WinAPI są tłumaczone na zrozumiałe komunikaty.
- UI pozostaje responsywne.
- `docs/SAFETY_NOTES.md` opisuje ograniczenia i ryzyka `TerminateProcess`.

## Build i test

Uruchom build Debug i Release.  
Przetestuj na `notepad.exe`.

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
