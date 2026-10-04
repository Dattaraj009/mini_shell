# Manual test cases

Run `make` and start `./mini-shell`, then try these cases:

1. `echo hello world` — prints both words.
2. `pwd`, then `cd /tmp`, then `pwd` — the second path is `/tmp`.
3. `ls -la /tmp` — passes multiple arguments to an external command.
4. `echo saved > mini-shell-test.txt` — creates/replaces the file.
5. `echo appended >> mini-shell-test.txt` — appends to the file.
6. `cat < mini-shell-test.txt` — reads the redirected file as standard input.
7. `echo beta | grep beta` — prints `beta`.
8. `sleep 2 &` — returns to the prompt before two seconds pass.
9. `history` — prints up to the last ten entered commands.
10. Run `sleep 20`, press Ctrl+C, then run `pwd` — the command stops and the
    shell continues.

Remove `mini-shell-test.txt` after testing if it is no longer needed.
