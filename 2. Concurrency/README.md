# adventure - CS 344

This is a text-driven adventure style game where the goal is to get to the end.

[http://en.wikipedia.org/wiki/Colossal_Cave_Adventure](http://en.wikipedia.org/wiki/Colossal_Cave_Adventure)

In the game, the player will begin in the "starting room" and will win the game automatically upon entering the "ending room", which causes the game to exit, displaying the path taken by the player.

During the game, the player can also enter a command that returns the current time - this functionality utilizes mutexes and multithreading.

To see the requirements for the project, read [req.md](https://github.com/IvanHalim/operating-systems/blob/master/3.%20Process%20Management/req.md).

### Compilation

To compile, type:
```
$ gcc –o halimi.buildrooms halimi.buildrooms.c
$ halimi.buildrooms
$ gcc –o halimi.adventure halimi.adventure.c -lpthread
$ halimi.adventure
```