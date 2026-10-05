# WPCC 0.1.14-rc.1 — poprawka autostartu (pre-release)

To wydanie testowe. Build i testy automatyczne są warunkiem publikacji, ale **nie potwierdzają działania autostartu po wylogowaniu lub restarcie**. Te dwa scenariusze nie zostały jeszcze sprawdzone na Windows 10/11 z rzeczywistym logowaniem użytkownika.

## Zmiany

- Autostart korzysta z Harmonogramu zadań zamiast wpisu Run, aby uruchamiać WPCC z wymaganymi uprawnieniami administratora po zalogowaniu właściwego użytkownika.
- Przełącznik pokazuje odczytany stan Windows, a błędy konfiguracji i zapisu ustawień są widoczne w interfejsie. Szybkie kolejne zmiany są zapisywane w kolejności.
- Zgodny, aktywny stary wpis jest migrowany. Brakujące lub wyłączone zadanie nie jest odtwarzane przy samym odczycie ustawień.
- Awaria ikony zasobnika pozostawia dostępne okno; po restarcie Explorera aplikacja odtwarza ikonę.
- Dodano diagnostykę startup.log, weryfikator zadania i usuwanie własnych wpisów autostartu przy odinstalowaniu.
- Poprawiono ścieżki pakowania i obsługę błędów narzędzi. GitHub Actions buduje Debug/Release, uruchamia testy oraz tworzy Setup i Portable z tej samej binarki Release.

## Pobieranie i ograniczenia

- Setup: `WindowsProcessControlCenter-v0.1.14-Setup.exe`.
- Portable: `WindowsProcessControlCenter-v0.1.14-Portable.zip` — rozpakuj całość, zachowując katalog `web` przy EXE.
- Wersja wewnętrzna EXE i instalatora: `0.1.14.0`; oznaczenie wydania GitHub: `v0.1.14-rc.1`.
- Autostart jest przeznaczony dla użytkownika zalogowanego na własne konto administratora. Podanie danych innego administratora w UAC nie konfiguruje autostartu dla pierwotnego użytkownika.
- Po przeniesieniu Portable wyłącz i ponownie włącz autostart. Paczki są niepodpisane cyfrowo i wymagają WebView2 Runtime.
- Testy logiki nie zastępują testów rzeczywistego logowania, migracji, awarii zasobnika ani odinstalowania z wieloma profilami użytkowników. Pełna macierz: `docs/AUTOSTART_FIX.md`.

## Krótki test na swoim komputerze

1. Uruchom nową wersję i włącz „Start with Windows”. Poczekaj na potwierdzenie bez ostrzeżenia. Opcjonalnie włącz minimalizowanie do zasobnika.
2. Wyloguj się i zaloguj ponownie. Po około 10–30 sekundach sprawdź, czy działa dokładnie jeden proces WPCC, jest dostępna ikona/okno i zachowane ustawienia.
3. Osobno uruchom ponownie komputer i powtórz sprawdzenie po zalogowaniu.
4. Wyłącz autostart, zamknij WPCC i ponownie się zaloguj: aplikacja nie powinna wystartować.
5. Jeśli coś nie działa, zachowaj `%LOCALAPPDATA%\WindowsProcessControlCenter\startup.log` oraz komunikat z ustawień. Skrypt `scripts/verify_startup.ps1` w Portable sprawdza konfigurację zadania, lecz nie zastępuje restartu.
