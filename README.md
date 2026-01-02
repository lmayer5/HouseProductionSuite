# Gradient Sound 🎹

A modern, ultralight e-commerce platform for premium audio plugins and VSTs.

## 🚀 Overview

Gradient Sound is a boutique audio technology studio offering simple, effective, and modern audio tools for music producers. Features a streamlined checkout with PDF invoice generation and GitHub-based digital delivery.

## 🛠️ Tech Stack

| Layer | Technologies |
| :--- | :--- |
| **Frontend** | HTML5, Tailwind CSS, DaisyUI, Alpine.js |
| **Backend** | PHP 8.x, PHPMailer |
| **Persistence** | JSON flat-file storage |
| **Delivery** | GitHub API (Private Repo Invitations) |

## 📁 Project Structure

```
.
├── public_html/          # Web root (point domain here)
│   ├── index.html        # Main storefront
│   ├── admin/            # Admin dashboard
│   ├── process_order.php # Order submission API
│   └── vendor/           # Composer dependencies
├── private_data/         # Sensitive data (NOT in Git)
│   ├── config.php        # Credentials
│   ├── settings.json     # Site settings
│   ├── orders.json       # Order database
│   └── invoices/         # PDF invoices
└── composer.json         # PHP dependencies
```

## 💻 Local Development

```bash
# 1. Clone the repo
git clone https://github.com/yourusername/gradient-solutions-site.git
cd gradient-solutions-site

# 2. Install dependencies
composer install

# 3. Create private_data folder and config
mkdir private_data
cp private_data.example/config.php private_data/config.php
# Edit config.php with your credentials

# 4. Start local server
php -S localhost:8000 -t public_html

# 5. Open http://localhost:8000
```

## 🚀 Deployment (Hostinger)

1.  **Upload Files:** Upload entire project to your hosting root.
2.  **Set Web Root:** Point your domain to `public_html/`.
3.  **Create `private_data/`:** Manually create folder above `public_html/`.
4.  **Configure `config.php`:**
    ```php
    <?php
    return [
        'ADMIN_PASSWORD' => 'your-secure-password',
        'SMTP_HOST' => 'smtp.hostinger.com',
        'SMTP_USER' => 'orders@gradientsound.shop',
        'SMTP_PASS' => 'your-email-password',
        'SMTP_PORT' => 587,
        'SMTP_FROM_EMAIL' => 'orders@gradientsound.shop',
        'SMTP_FROM_NAME' => 'Gradient Sound',
        'ADMIN_EMAIL' => 'admin@gradientsound.shop',
    ];
    ```
5.  **Install Composer:** Run `composer install` in root directory.
6.  **Set Permissions:** `chmod 755 private_data && chmod 644 private_data/*`

## 🔐 Security Notes

-   `private_data/` is excluded from Git and should NEVER be committed.
-   Admin dashboard is password-protected via session.
-   SMTP credentials are stored server-side only.

## © License

© 2025 Gradient Sound. All rights reserved.
