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
   `R<username>` είτε `U<username>
   1. Στην περίπτωση της πρώτης μορφής `R<username>`:
      1. Αν ο χρήστης έχει εγγραφεί ήδη στον server τότε στέλνετε response πίσω στον client
         της μορφής `409CONFLICT` και κλείνει τη σύνδεση με τον client
      2. Αν ο χρήστης δεν υπάρχει τότε τον εγγράφει στη λίστα με τους registered users
   2. Στην περίπτωση της δεύτερης μορφής `U<username>`:
      1. Αν ο χρήστης έχει εγγραφεί ήδη στον server τότε τον διαγράφει απο τη λίστα
         με τους registered users και στέλνει response `200OK` και κλείνει τη σύνδεση με τον client
      2. Αν ο χρήστης δεν υπάρχει τότε στέλνετε response πίσω στον client
         της μορφής `404NOTFOUND` και κλείνει τη σύνδεση με τον client
3. Στη συνέχεια αν το πρώτο μήνυμα ήταν της μορφής `register` τότε ο server περιμένει για το δεύτερο (operation)
   μήνυμα από τον client που είναι είτε της μορφής `L <IP> <PORT>` είτε της μορφής `C <username>`
   1. Στην πρώτη περίπτωση ανανεώνονται τα στοιχεία του `user` που δημιουργήθηκε στο **βήμα (1.ii)** σε mode **listen**
      και την IP και το PORT
   2. Στη δεύτερη περίπτωση γίνεται αναζήτηση αν υπάρχει ο συγκεκριμένος `<username>` στη λίστα και τότε:
      1. Αν υπάρχει τότε στέλνουμε response πίσω στον client της μορφής `200OK <IP> <PORT>`
      2. Aν δεν υπάρχει τότε στέλνουμε response πίσω στον client της μορφής `404ΝOTFOUND`


### Client

1. O client κάνει connect στον server `127.0.0.1:29000` και στέλνει το πρώτο μήνυμα της μορφής `R<username>`
2. Αν εγγραφεί επιτυχώς τότε ανάλογα με το `--mode` flag που είναι είτε `listen` είτε `connect`
   1. Στο `listen` στέλνει το operation message της μορφής `L <IP> <PORT>` και κλείνει τη σύνδεση με τον server
      1. Έπειτα ανοίγει καινούργιο socket σε `listening mode` και περιμένει για χρήστες να συνδεθούν για chat
      2. Ότι μήνυμα λαμβάνει το στέλνει πίσω ώς έχει
      3. Όταν τερματίζει το πρόγραμμα (πατώντας CTRL + C) τότε ξανασυνδέεται με τον server και στέλνει μήνυμα
         της μορφής `U<username>` για να διαγραφεί απο τη λίστα με τους registered users
   2. Στο `connect` στέλνει το operation message της μορφής `C <username>` και περιμένει απάντηση
      1. Αν ο συγκεκριμένος `<username>` δεν υπάρχει στη λίστα τότε κλείνει τη σύνδεση με τον server και κλείνει το πρόγραμμα
      2. Αν υπάρχει τότε λαμβάνει response από τον server της μορφής `200OK <IP> <PORT>` και συνδέεται στον 
         `listening client`
         1. Αφού γίνει η σύνδεση τότε μπορούμε να κάνουμε chat (αποστολή μηνυμάτων) μέχρις ότου να γράψουμε το μήνυμα
            'Q' ή 'q'
         2. Όταν κλείσει η σύνδεση με τον `listening client` τότε ξανασυνδέεται στον server και στέλνει μήνυμα
            της μορφής `U<username>` για να διαγραφεί απο τη λίστα με τους registered users


#### Περιορισμοί πρωτοκόλλου

Το μέγιστο μέγεθος μηνύματος είναι 2048 bytes
Το μέγιστο μέγεθος `<username>` είναι 256 bytes