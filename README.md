# C Chat - Γαζής Γιώργος - Βασιλείου Κώστας

Το project χρησιμοποιεί το tool cmake για να κάνει compile
όλα τα απαραίτητα executables και libraries.

Αρχικά πρέπει να κάνουμε build το project ακολουθώντας την
παρακάτω μέθοδο

### Build CMake με Makefile

Από το root directory (c-chat) τρέχουμε την εντολή
```bash
make
```
Ή τις εντολές (με αυτήν την σειρά)
```bash
make clean
make configure
make build-debug
```

### Build CMake χωρίς Makefile

Aφού βεβαιωθούμε ότι υπάρχει ο φάκελος build και είναι άδειος στο
root directory (c-chat) τρέχουμε τις εντολές (με αυτή τη σειρά) 

```bash
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/bin/gcc -G "Unix Makefiles" -S . -B build/

/usr/bin/cmake --build ./build/  --target all   -- -j 6
```

## Χρήση Προγράμματος

Στη συνέχεια αφού γίνει το build μπορούμε να χρησιμοποιήσουμε τα executables που
έχουν δημιουργηθεί στον φάκελο `./build/apps` τα οποία είναι τα:

- **server**
- **client**

Η σειρά των εκτελέσιμων αρχείων που πρέπει να ακολουθηθεί είναι (υποθέτοντας ότι βρισκόμαστε)
στο root directory (c-chat) του project

1. Ξεκινάμε έναν server με την εντολή 
    ```bash
   ./build/apps/server 
   ```
   είτε μέσω του makefile κανόνα με την εντολή
    ```bash
   make run-server 
   ```
2. Ξεκινάμε έναν client που θα κάνει listen με την εντολή
    ```bash
   ./build/apps/client --mode listen --username <the_user_name>
   ```
    Αντικαθιστώντας τη μεταβλητή `<the_user_name>` με το όνομα του
    user που θέλουμε να οριστεί
    
    Για ευκολία έχει δημιουργηθεί ένας makefile κανόνας
    όπου όταν τον τρέξουμε (αντί για την παραπάνω εντολή)
    ξεκινάει αυτόματα έναν client σε **listening mode**
    με αυτόματη συμπλήρωση των flags
    ```bash
   make run-client-listen
   ```
3. Ξεκινάμε έναν client που θα κάνει connect με έναν ήδη υπάρχων 
   client που κάνει **listen**, με την εντολή
    ```bash
   ./build/apps/client --mode listen --username <the_user_name> --client-username <the_user_name_that_already_listens>
   ```
    Αντικαθιστώντας τη μεταβλητή `<the_user_name>` όπως στο **βήμα (2.)** και 
    τη μεταβλητή `<the_user_name_that_already_listens>` με την το όνομα του χρήστη
    που θέλουμε να συνδεθούμε (και ακούει ήδη για συνδέσεις --> **βήμα (2.)**
    
    Και εδώ για ευκολία έχει δημιουργηθεί ένας makefile κανόνας
    όπου όταν τον τρέξουμε (αντί για την παραπάνω εντολή)
    ξεκινάει αυτόματα έναν client σε **connect mode**
    με αυτόματη συμπλήρωση των flags
    ```bash
   make run-client-connect
   ```

## Πρωτόκολλο

### Server

1. O server περιμένει για συνδέσεις στη διεύθυνση `127.0.0.1:29000`
2. Αφού συνδεθεί κάποιος χρήστης ο server περιμένει για το πρώτο μήνυμα που είναι της μορφής
    