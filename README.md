# Martini Golf Tees Canada ⛳

A lightweight, high-performance e-commerce site for Martini Golf Tees, built with PHP, Alpine.js, and Tailwind CSS.

## 📁 Project Structure

- **`public_html/`**: The web root. Contains all public-facing files (`index.html`, PHP scripts, assets).
- **`private_data/`**: (Ignored) Contains sensitive data like `orders.json`, `settings.json`, and generated invoices. **Do not upload this directory to public web access.**
- **`config.php`**: (Ignored) Contains sensitive credentials (SMTP, Admin Password).

## 🚀 Setup

1. **Dependencies**: Run `composer install` to install PHPMailer and other dependencies.
2. **Configuration**: 
   - Rename `config.example.php` (if exists) or create a `config.php` in the root.
   - Define the following constants:
     ```php
     define('ADMIN_PASSWORD', 'your_secure_password');
     define('SMTP_HOST', 'smtp.hostinger.com');
     define('SMTP_USER', 'your_email@domain.com');
     define('SMTP_PASS', 'your_email_password');
     define('SMTP_PORT', 587);
     define('SMTP_FROM_EMAIL', 'orders@domain.com');
     define('SMTP_FROM_NAME', 'Martini Tees Orders');
     define('ADMIN_EMAIL', 'admin@domain.com');
     ```
3. **Deployment**:
   - Upload the contents of `public_html` to your server's `public_html`.
   - Create a `private_data` directory *outside* the web root (one level up).
   - Ensure `private_data` is writable by the web server.

## 🛠️ Tech Stack

- **Frontend**: HTML5, Tailwind CSS (via CDN), DaisyUI, Alpine.js.
- **Backend**: PHP 8.x.
- **Data**: JSON-based flat-file storage (NoSQL/Database-free).

## © Copyright

© 2025 Martini Golf Tees, Inc. | "Martini Golf Tees" is a registered trademark of Martini Golf Tees, Inc.
