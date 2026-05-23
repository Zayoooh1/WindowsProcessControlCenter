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


# Task 11 — Finalny polish i QA

## Cel

Doprowadź aplikację do stanu, w którym jest przyjemnym, stabilnym MVP, a nie tylko technicznym demo.

## Zakres

Przejrzyj cały projekt i popraw:

- crashe,
- warningi kompilatora,
- memory/resource leaks,
- niezamknięte uchwyty,
- migotanie UI,
- layout overflow,
- dziwne komunikaty błędów,
- niespójne nazwy,
- niepotrzebnie agresywne akcje,
- martwy kod,
- brakujące include,
- braki w README.

## UX finalny

Sprawdź:

- czy najczęstsze akcje są łatwo dostępne,
- czy ryzykowne akcje nie są zbyt łatwe do kliknięcia,
- czy użytkownik rozumie, co zrobi GPU preference,
- czy Realtime jest odpowiednio ostrzeżony,
- czy Freeze/Resume jest opisane z sensem,
- czy tabela procesów jest czytelna,
- czy aplikacja ma dobrą domyślną szerokość kolumn.

## Dokumentacja finalna

README musi zawierać:

- czym jest aplikacja,
- wymagania systemowe,
- jak zbudować,
- jak uruchomić jako admin,
- opis funkcji,
- ograniczenia,
- ostrzeżenie o Realtime,
- ostrzeżenie o Freeze,
- ostrzeżenie o GPU preference,
- brak telemetrii,
- troubleshooting.

## Kryteria ukończenia

- Build Debug i Release przechodzą.
- Testy przechodzą.
- Aplikacja działa na zwykłym koncie i jako admin.
- Nie ma oczywistych problemów UI.
- `dist/` zawiera gotowe MVP.
- `docs/PROGRESS.md` ma finalne podsumowanie.

## Build i test

Uruchom pełny build i testy:

```powershell
./scripts/build.ps1 -Configuration Release
ctest --test-dir build --output-on-failure
```

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
