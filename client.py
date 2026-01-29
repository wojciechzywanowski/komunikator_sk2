import socket
import threading
import tkinter as tk
from tkinter import simpledialog, messagebox
import sys

if len(sys.argv) != 3:
    print("Użycie: python3 client.py <adres_ip> <port>")
    sys.exit(1)

HOST = sys.argv[1]
PORT = int(sys.argv[2])


class ChatClient:
    def __init__(self, root):
        self.root = root
        self.root.title("Komunikator")

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((HOST, PORT))

        self.username = None

        self.login_screen()
        threading.Thread(target=self.receive_messages, daemon=True).start()

    def login_screen(self):
        self.clear_window()

        tk.Label(self.root, text="Login").pack()
        self.login_entry = tk.Entry(self.root)
        self.login_entry.pack()

        tk.Label(self.root, text="Hasło").pack()
        self.pass_entry = tk.Entry(self.root, show="*")
        self.pass_entry.pack()

        tk.Button(self.root, text="Zaloguj", command=self.login).pack(pady=5)
        tk.Button(self.root, text="Zarejestruj", command=self.register).pack(pady=5)

    def login(self):
        login = self.login_entry.get()
        password = self.pass_entry.get()
        self.username = login
        self.send_cmd(f"LOGIN {login} {password}")

    def register(self):
        login = self.login_entry.get()
        password = self.pass_entry.get()
        self.send_cmd(f"REGISTER {login} {password}")

    def chat_screen(self):
        self.clear_window()

        tk.Label(self.root, text=f"Zalogowany jako: {self.username}",
                 font=("Arial",12,"bold")).pack(pady=5)

        main = tk.Frame(self.root)
        main.pack(expand=True, fill=tk.BOTH)

        left = tk.Frame(main, width=150)
        left.pack(side=tk.LEFT, fill=tk.Y)

        tk.Label(left, text="Twoje grupy").pack()
        self.group_list = tk.Listbox(left)
        self.group_list.pack(fill=tk.BOTH, expand=True)

        right = tk.Frame(main)
        right.pack(side=tk.RIGHT, expand=True, fill=tk.BOTH)

        self.chat_area = tk.Text(right, state='disabled')
        self.chat_area.pack(expand=True, fill=tk.BOTH)

        bottom = tk.Frame(right)
        bottom.pack(fill=tk.X)

        self.msg_entry = tk.Entry(bottom)
        self.msg_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)

        tk.Button(bottom, text="Wyślij PW", command=self.send_private_message).pack(side=tk.LEFT)
        tk.Button(bottom, text="Utwórz grupę", command=self.create_group).pack(side=tk.LEFT)
        tk.Button(bottom, text="Dołącz do grupy", command=self.join_group).pack(side=tk.LEFT)
        tk.Button(bottom, text="Wyślij do grupy", command=self.send_group_message).pack(side=tk.LEFT)

    def send_cmd(self, cmd):
        self.sock.sendall((cmd + "\n").encode())

    def send_private_message(self):
        target = simpledialog.askstring("PW", "Do kogo?")
        msg = self.msg_entry.get()
        if target and msg:
            self.send_cmd(f"MSG {target} {msg}")
            self.write_chat(f"[JA -> {target}]: {msg}")
            self.msg_entry.delete(0, tk.END)

    def create_group(self):
        name = simpledialog.askstring("Grupa", "Nazwa grupy:")
        if name:
            self.send_cmd(f"CREATE_GROUP {name}")
            self.group_list.insert(tk.END, name)

    def join_group(self):
        name = simpledialog.askstring("Grupa", "Nazwa grupy:")
        if name:
            self.send_cmd(f"JOIN_GROUP {name}")
            self.group_list.insert(tk.END, name)

    def send_group_message(self):
        selection = self.group_list.curselection()
        if not selection:
            messagebox.showerror("Błąd", "Wybierz grupę z listy!")
            return

        group = self.group_list.get(selection[0])
        msg = self.msg_entry.get()
        if msg:
            self.send_cmd(f"GROUP_MSG {group} {msg}")
            self.write_chat(f"[JA | {group}]: {msg}")
            self.msg_entry.delete(0, tk.END)

    def receive_messages(self):
        while True:
            try:
                data = self.sock.recv(2048).decode()
                if not data:
                    break
                if "Zalogowano" in data:
                    self.chat_screen()
                self.write_chat(data.strip())
            except:
                break

    def write_chat(self, msg):
        if hasattr(self, "chat_area"):
            self.chat_area.config(state='normal')
            self.chat_area.insert(tk.END, msg + "\n")
            self.chat_area.yview(tk.END)
            self.chat_area.config(state='disabled')

    def clear_window(self):
        for widget in self.root.winfo_children():
            widget.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    root.geometry("900x500")
    app = ChatClient(root)
    root.mainloop()
