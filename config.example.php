<?php
// private_data/config.php
// EXAMPLE CONFIGURATION - Copy this to private_data/config.php and fill in your values

return [
    // Admin Dashboard Password
    'ADMIN_PASSWORD' => 'CHANGE_THIS_TO_A_SECURE_PASSWORD',

    // SMTP Email Configuration (for Hostinger)
    'SMTP_HOST' => 'smtp.hostinger.com',
    'SMTP_USER' => 'orders@gradientsound.shop',
    'SMTP_PASS' => 'YOUR_EMAIL_PASSWORD',
    'SMTP_PORT' => 587,
    'SMTP_FROM_EMAIL' => 'orders@gradientsound.shop',
    'SMTP_FROM_NAME' => 'Gradient Sound',

    // Admin Notification Email
    'ADMIN_EMAIL' => 'admin@gradientsound.shop',

    // GitHub Delivery (Optional - Can also be set in Admin Dashboard)
    // 'GITHUB_PAT' => 'ghp_xxxxxxxxxxxxxxxxxxxx',
    // 'GITHUB_REPO' => 'username/repository',
];
