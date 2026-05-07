package main

import (
	"bufio"
	"fmt"
	"html/template"
	"net/http"
	"net/smtp"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"time"
)

func isValidBirthDate(birth string) bool {
	if len(birth) != 10 || birth[4] != '-' || birth[7] != '-' {
		return false
	}
	year, err1 := strconv.Atoi(birth[0:4])
	month, err2 := strconv.Atoi(birth[5:7])
	day, err3 := strconv.Atoi(birth[8:10])
	if err1 != nil || err2 != nil || err3 != nil {
		return false
	}
	currentYear := time.Now().Year()
	if year < currentYear-150 || year > currentYear-2 {
		return false
	}
	if month < 1 || month > 12 {
		return false
	}
	if day < 1 || day > 31 {
		return false
	}
	return true
}

func sendEmail(to string, userName string) {
	from := "detective14k@gmail.com"
	password := "KDetective_15"
	smtpHost := "smtp.gmail.com"
	smtpPort := "587"

	subject := "Subject: MySite Registration\n"
	mime := "MIME-version: 1.0;\nContent-Type: text/html; charset=\"UTF-8\";\n\n"
	body := "<html><body><h2>Привет, " + userName + "!</h2><p>Вы успешно зарегистрировались.</p></body></html>"
	message := []byte(subject + mime + body)

	auth := smtp.PlainAuth("", from, password, smtpHost)
	_ = smtp.SendMail(smtpHost+":"+smtpPort, auth, from, []string{to}, message)
}

type User struct {
	Login    string
	Email    string
	Password string
	Name     string
	Birth    string
}

type PageData struct {
	LoggedIn bool
	UserName string
	Error    string
}

func trim(s string) string {
	return strings.TrimSpace(s)
}

func loadUsers() ([]User, error) {
	users := []User{}
	file, err := os.Open("db.txt")
	if err != nil {
		if os.IsNotExist(err) {
			return users, nil
		}
		return nil, err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	re := regexp.MustCompile(`User: *(.+?) *\| *Email: *(.+?) *\| *Pass: *(.+?) *\| *Name: *(.+?) *\| *Birth: *(.+?)$`)
	for scanner.Scan() {
		line := scanner.Text()
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}
		matches := re.FindStringSubmatch(line)
		if len(matches) == 6 {
			users = append(users, User{
				Login:    trim(matches[1]),
				Email:    trim(matches[2]),
				Password: trim(matches[3]),
				Name:     trim(matches[4]),
				Birth:    trim(matches[5]),
			})
		}
	}
	return users, scanner.Err()
}

func getUser(login string) (*User, error) {
	users, err := loadUsers()
	if err != nil {
		return nil, err
	}
	for _, user := range users {
		if user.Login == login {
			return &user, nil
		}
	}
	return nil, nil
}

func authenticate(login, password string) (*User, error) {
	user, err := getUser(login)
	if err != nil {
		return nil, err
	}
	if user != nil && user.Password == password {
		return user, nil
	}
	return nil, nil
}

func renderPage(w http.ResponseWriter, page string, data PageData) {
	tmpl, err := template.ParseFiles(filepath.Join("templates", page+".html"))
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	tmpl.Execute(w, data)
}

func getSessionUser(r *http.Request) string {
	cookie, err := r.Cookie("session_user")
	if err != nil {
		return ""
	}
	return cookie.Value
}

func pageData(r *http.Request) PageData {
	login := getSessionUser(r)
	if login == "" {
		return PageData{LoggedIn: false}
	}
	user, err := getUser(login)
	if err != nil || user == nil {
		return PageData{LoggedIn: false}
	}
	return PageData{LoggedIn: true, UserName: user.Name}
}

func main() {
	fs := http.FileServer(http.Dir("static"))
	http.Handle("/static/", http.StripPrefix("/static/", fs))

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		data := pageData(r)
		renderPage(w, "index", data)
	})

	http.HandleFunc("/login", func(w http.ResponseWriter, r *http.Request) {
		if r.Method == http.MethodPost {
			login := r.FormValue("login")
			password := r.FormValue("password")
			user, err := authenticate(login, password)
			if err != nil {
				http.Error(w, "Server error", http.StatusInternalServerError)
				return
			}
			if user == nil {
				renderPage(w, "login", PageData{LoggedIn: false, Error: "Account not found. Please register."})
				return
			}
			cookie := &http.Cookie{Name: "session_user", Value: user.Login, Path: "/", MaxAge: 3600}
			http.SetCookie(w, cookie)
			http.Redirect(w, r, "/", http.StatusSeeOther)
			return
		}
		renderPage(w, "login", pageData(r))
	})

	http.HandleFunc("/register", func(w http.ResponseWriter, r *http.Request) {
		if r.Method == http.MethodPost {
			login := r.FormValue("login")
			email := r.FormValue("email")
			password := r.FormValue("password")
			name := r.FormValue("name")
			birth := r.FormValue("birth")

			// Validate birth date
			if !isValidBirthDate(birth) {
				renderPage(w, "register", PageData{LoggedIn: false, Error: "Please enter a realistic birth date (between 2 and 150 years old)."})
				return
			}

			userData := fmt.Sprintf("User: %s | Email: %s | Pass: %s | Name: %s | Birth: %s\n", login, email, password, name, birth)
			f, err := os.OpenFile("db.txt", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
			if err == nil {
				f.WriteString(userData)
				f.Close()
			}
			cookie := &http.Cookie{Name: "session_user", Value: login, Path: "/", MaxAge: 3600}
			http.SetCookie(w, cookie)
			go sendEmail(email, login)
			http.Redirect(w, r, "/", http.StatusSeeOther)
			return
		}
		renderPage(w, "register", pageData(r))
	})

	http.HandleFunc("/logout", func(w http.ResponseWriter, r *http.Request) {
		cookie := &http.Cookie{Name: "session_user", Value: "", Path: "/", MaxAge: -1}
		http.SetCookie(w, cookie)
		http.Redirect(w, r, "/", http.StatusSeeOther)
	})

	http.HandleFunc("/download", func(w http.ResponseWriter, r *http.Request) {
		renderPage(w, "download", pageData(r))
	})

	http.ListenAndServe(":8080", nil)
}