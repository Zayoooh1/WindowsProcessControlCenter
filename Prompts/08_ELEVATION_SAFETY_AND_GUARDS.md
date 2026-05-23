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


# Task 08 — Elevation, UAC i zabezpieczenia przed głupimi akcjami

## Cel

Dodaj solidny system uprawnień, ostrzeżeń i blokad, żeby aplikacja była użyteczna, ale nie pozwalała przypadkowo rozwalić systemu.

## Zakres

Rozszerz:

```text
src/core/PrivilegeHelper.h
src/core/PrivilegeHelper.cpp
src/core/ProcessActions.cpp
src/core/PriorityManager.cpp
src/core/SuspensionManager.cpp
src/ui/UiShell.cpp
```

## Funkcje

Dodaj:

- wykrywanie, czy aplikacja działa jako administrator,
- przycisk/rekomendację „Restart as administrator”,
- wykrywanie integrity level procesu, jeśli realnie potrzebne,
- centralną funkcję `CanPerformAction(process, action)`,
- klasyfikację procesów:
  - own process,
  - user process,
  - elevated process,
  - service/system-like,
  - critical/protected/inaccessible,
  - unknown.

## Reguły

- Akcje niedostępne mają mieć powód.
- Nie pokazuj samego „failed”.
- Przykład: „Access denied: run this app as administrator” albo „Blocked: system-critical process”.
- Nie proś o admina przy samym oglądaniu listy procesów.
- Admin ma pomagać przy akcjach, ale nie ma usuwać wszystkich blokad bezpieczeństwa.
- Nawet jako admin nie pozwalaj łatwo zamrażać/zamykać krytycznych procesów.

## Dialogi potwierdzeń

Dodaj osobne potwierdzenia dla:

- Terminate process,
- Freeze process,
- Realtime priority,
- operacji na procesach elevated/system-like.

Dialog ma być krótki:

- co zostanie zrobione,
- potencjalne ryzyko,
- przyciski: Cancel / Confirm.

## Dokumentacja

Uzupełnij:

```text
docs/SAFETY_NOTES.md
```

Opisz:

- dlaczego niektóre procesy są zablokowane,
- kiedy trzeba uruchomić jako administrator,
- dlaczego Realtime może być ryzykowny,
- dlaczego Freeze może zawiesić aplikację docelową,
- dlaczego GPU preference może wymagać restartu programu.

## Kryteria ukończenia

- Aplikacja jasno pokazuje status admin/user.
- Akcje niedostępne mają widoczny powód.
- Ryzykowne akcje mają potwierdzenia.
- Nie da się przypadkowo zabić/zamrozić własnej aplikacji.
- `docs/SAFETY_NOTES.md` jest konkretne i uczciwe.

## Build i test

Uruchom build Debug i Release.  
Przetestuj jako zwykły użytkownik i jako administrator.

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
