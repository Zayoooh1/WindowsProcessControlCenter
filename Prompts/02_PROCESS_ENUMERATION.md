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


# Task 02 — Enumeracja aktywnych procesów

## Cel

Zastąp mockowe dane realną listą aktywnych procesów w Windowsie.

## Zakres

Dodaj moduł:

```text
src/core/ProcessInfo.h
src/core/ProcessEnumerator.h
src/core/ProcessEnumerator.cpp
src/core/ErrorUtils.h
src/core/ErrorUtils.cpp
```

## Dane procesu

Tabela ma pokazywać minimum:

- nazwa procesu,
- PID,
- parent PID, jeśli dostępne,
- pełna ścieżka `.exe`, jeśli dostępna,
- architektura procesu: x64 / x86 / unknown,
- użycie RAM,
- klasa priorytetu CPU,
- status: running / inaccessible / protected / suspended-like / unknown,
- informacja, czy proces jest prawdopodobnie systemowy/krytyczny,
- session ID,
- nazwa użytkownika, jeśli da się pobrać bez agresywnego grzebania.

## Implementacja

Użyj stabilnych WinAPI:

- `CreateToolhelp32Snapshot` + `Process32First/Process32Next` do listy procesów,
- `OpenProcess` z minimalnymi potrzebnymi prawami,
- `QueryFullProcessImageNameW` do ścieżki,
- `GetPriorityClass`,
- `GetProcessMemoryInfo`,
- `IsWow64Process2`, jeśli dostępne,
- poprawne mapowanie błędów `GetLastError`.

## Ważne ograniczenia

- Nie traktuj `Access Denied` jako błąd krytyczny aplikacji.
- Procesy chronione mają być widoczne, ale oznaczone jako niedostępne.
- Nie crashuj na procesach, które znikną w trakcie odświeżania.
- Snapshot procesu może się zmieniać — kod musi być odporny na race condition.

## UI

Dodaj:

- pole wyszukiwania,
- przycisk Refresh,
- auto-refresh co 2 sekundy z możliwością wyłączenia,
- sortowanie po PID, nazwie, RAM, priorytecie,
- czytelne puste stany i komunikaty błędów.

## Kryteria ukończenia

- Aplikacja pokazuje realne procesy.
- Odświeżanie nie zawiesza UI.
- Procesy niedostępne są pokazane, ale nie rozwalają logiki.
- Tabela jest czytelna przy dużej liczbie procesów.
- `docs/PROGRESS.md` jest zaktualizowany.

## Build i test

Uruchom build Debug i Release.  
Przetestuj na zwykłym koncie użytkownika oraz po uruchomieniu jako administrator.

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
