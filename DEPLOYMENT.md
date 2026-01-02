# Hostinger Deployment Guide 🚀

A step-by-step guide to deploying Gradient Sound on Hostinger shared hosting.

---

## Prerequisites

- Hostinger hosting account (Business or Premium plan recommended)
- Domain pointed to Hostinger nameservers
- FTP client (FileZilla) or Hostinger File Manager
- SSH access (optional but recommended)

---

## Step 1: Prepare Your Files

Before uploading, ensure your local project has:

```
gradient_solutions_site/
├── public_html/          ✅ Upload this
├── composer.json         ✅ Upload this
├── config.example.php    ✅ Upload this (for reference)
├── private_data/         ❌ DO NOT upload (recreate on server)
└── vendor/               ❌ DO NOT upload (install via Composer)
```

---

## Step 2: Access Hostinger File Manager

1. Log in to [Hostinger hPanel](https://hpanel.hostinger.com)
2. Select your domain
3. Go to **Files** → **File Manager**
4. Navigate to `public_html` (this is your web root)

---

## Step 3: Upload Files

### Option A: File Manager (Simple)

1. Delete default `index.html` in `public_html`
2. Upload contents of your local `public_html/` folder directly into Hostinger's `public_html/`
3. Upload `composer.json` to the **parent** of `public_html/` (one level up)

### Option B: FTP (Faster for large uploads)

1. Get FTP credentials from hPanel → **Files** → **FTP Accounts**
2. Connect via FileZilla:
   - Host: `ftp.gradientsound.shop`
   - Username: Your FTP username
   - Password: Your FTP password
   - Port: `21`
3. Upload files as described above

---

## Step 4: Create `private_data` Directory

This folder stores sensitive data and must be **outside** `public_html`:

1. In File Manager, navigate to root (`/`)
2. Create folder: `private_data`
3. Inside `private_data`, create:
   - `config.php` (copy content below)
   - `settings.json` (empty: `{}`)
   - `orders.json` (empty: `[]`)
   - `invoices/` folder

### config.php Content:

```php
<?php
return [
    'ADMIN_PASSWORD' => 'YOUR_SECURE_PASSWORD_HERE',
    
    'SMTP_HOST' => 'smtp.hostinger.com',
    'SMTP_USER' => 'orders@gradientsound.shop',
    'SMTP_PASS' => 'YOUR_EMAIL_PASSWORD',
    'SMTP_PORT' => 587,
    'SMTP_FROM_EMAIL' => 'orders@gradientsound.shop',
    'SMTP_FROM_NAME' => 'Gradient Sound',
    
    'ADMIN_EMAIL' => 'admin@gradientsound.shop',
];
```

---

## Step 5: Install Composer Dependencies

### Option A: SSH (Recommended)

1. Enable SSH in hPanel → **Advanced** → **SSH Access**
2. Connect via terminal:
   ```bash
   ssh u123456789@gradientsound.shop -p 65002
   ```
3. Navigate and install:
   ```bash
   cd domains/gradientsound.shop
   composer install --no-dev
   ```

### Option B: No SSH Access

1. Run `composer install` locally
2. Upload the entire `vendor/` folder to your server (inside `public_html/`)

---

## Step 6: Set File Permissions

Via SSH or File Manager, set:

| Path | Permission |
| :--- | :--- |
| `private_data/` | 755 |
| `private_data/config.php` | 644 |
| `private_data/orders.json` | 666 |
| `private_data/settings.json` | 666 |
| `private_data/invoices/` | 755 |

SSH commands:
```bash
chmod 755 ~/domains/gradientsound.shop/private_data
chmod 644 ~/domains/gradientsound.shop/private_data/config.php
chmod 666 ~/domains/gradientsound.shop/private_data/*.json
chmod 755 ~/domains/gradientsound.shop/private_data/invoices
```

---

## Step 7: Create Email Account

1. Go to hPanel → **Emails** → **Email Accounts**
2. Create: `orders@gradientsound.shop`
3. Note the password (use in `config.php`)
4. SMTP settings for Hostinger:
   - Host: `smtp.hostinger.com`
   - Port: `587`
   - Encryption: `STARTTLS`

---

## Step 8: Configure SSL

1. Go to hPanel → **Security** → **SSL**
2. Enable **Free SSL** for your domain
3. Wait 10-15 minutes for activation
4. Enable **Force HTTPS** redirect

---

## Step 9: Test Your Site

1. Visit `https://gradientsound.shop` - Storefront should load
2. Visit `https://gradientsound.shop/admin/` - Login with your password
3. Place a test order
4. Check admin dashboard for the order
5. Verify email delivery

---

## Troubleshooting

### "500 Internal Server Error"
- Check `private_data/` exists and has correct permissions
- Verify `composer install` was run
- Check PHP error logs in hPanel

### Emails not sending
- Verify SMTP credentials in `config.php`
- Check email account exists and password is correct
- Try sending a test email from Hostinger webmail first

### Orders not saving
- Check `orders.json` has write permission (666)
- Verify `private_data/` path is correct in PHP files

### Admin login not working
- Clear browser cookies
- Verify password in `config.php` matches what you're entering

---

## File Structure on Server

After deployment, your server should look like:

```
/home/u123456789/
├── domains/
│   └── yourdomain.com/
│       ├── public_html/           ← Web root
│       │   ├── index.html
│       │   ├── admin/
│       │   ├── process_order.php
│       │   ├── vendor/
│       │   └── ...
│       ├── private_data/          ← Sensitive (outside public_html)
│       │   ├── config.php
│       │   ├── settings.json
│       │   ├── orders.json
│       │   └── invoices/
│       └── composer.json
```

---

## Quick Reference

| Item | Value |
| :--- | :--- |
| **Web Root** | `/public_html/` |
| **Config Location** | `/private_data/config.php` |
| **Admin URL** | `https://yourdomain.com/admin/` |
| **SMTP Host** | `smtp.hostinger.com` |
| **SMTP Port** | `587` |
| **PHP Version** | 8.0+ (set in hPanel → PHP Configuration) |

---

© 2025 Gradient Solutions
