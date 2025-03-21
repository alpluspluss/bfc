+++++ +++++             // Cell 0: Initialize counter to 10
[                       // Start loop
    > +++++ ++          // Cell 1: Add 7
    > +++++ +++++       // Cell 2: Add 10
    > +++               // Cell 3: Add 3
    > +                 // Cell 4: Add 1
    <<<< -              // Return to Cell 0 and decrement counter
]                       // End loop when counter is 0
> ++ .                  // Cell 1: 70 + 2 = 72 (H)
> + .                   // Cell 2: 100 + 1 = 101 (e)
+++++ ++ .              // Cell 2: 101 + 7 = 108 (l)
.                       // Cell 2: 108 (l)
+++ .                   // Cell 2: 108 + 3 = 111 (o)
> ++ .                  // Cell 3: 30 + 2 = 32 (space)
<< +++++ +++++ +++++ .  // Cell 1: 72 + 15 = 87 (W)
> .                     // Cell 2: 111 (o)
+++ .                   // Cell 2: 111 + 3 = 114 (r)
----- - .               // Cell 2: 114 - 6 = 108 (l)
----- --- .             // Cell 2: 108 - 8 = 100 (d)
> + .                   // Cell 3: 32 + 1 = 33 (!)