<?php
use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\Exception;

require 'vendor/autoload.php';

// Set JSON response header
header('Content-Type: application/json');

function sendResponse($success, $message) {
    echo json_encode(['success' => $success, 'message' => $message]);
    exit;
}

// 1. Validate Input
$id = $_POST['id'] ?? null;
$newStatus = $_POST['status'] ?? null;

// Require standard environment loader
require_once 'env_loader.php';

if (!$id || !$newStatus) {
    sendResponse(false, 'Missing required fields.');
}

// 2. Read Orders
// SECURE PATH: Move out of public_html
$ordersFile = __DIR__ . '/../private_data/orders.json';
if (!file_exists($ordersFile)) {
    sendResponse(false, 'Order database not found.');
}

$content = file_get_contents($ordersFile);
$orders = json_decode($content, true);

if (!is_array($orders)) {
    sendResponse(false, 'Order database invalid.');
}

// 3. Find and Update Order
$orderIndex = null;
$customerEmail = null;
$customerName = null;

foreach ($orders as $key => $order) {
    if ($order['id'] === $id) {
        $orderIndex = $key;
        $customerEmail = $order['customer']['email'];
        $customerName = $order['customer']['name'];
        break;
    }
}

if ($orderIndex === null) {
    sendResponse(false, 'Order not found.');
}

// Update Status
$orders[$orderIndex]['status'] = $newStatus;
$orders[$orderIndex]['updated_at'] = date('c');

// Save to File
if (file_put_contents($ordersFile, json_encode($orders, JSON_PRETTY_PRINT)) === false) {
    sendResponse(false, 'Failed to save database.');
}

// 4. Send Email Notification
if ($newStatus === 'Shipped') {
    $mail = new PHPMailer(true);
    try {
        // Server settings
        $mail->isSMTP();
        $mail->Host       = get_config('SMTP_HOST') ?: 'smtp.hostinger.com';
        $mail->SMTPAuth   = true;
        $mail->Username   = get_config('SMTP_USER') ?: 'user@example.com';
        $mail->Password   = get_config('SMTP_PASS') ?: 'secret';
        $mail->SMTPSecure = PHPMailer::ENCRYPTION_STARTTLS;
        $mail->Port       = get_config('SMTP_PORT') ?: 587;
        $mail->Timeout    = 5;

        // Check for placeholder credentials
        if (get_config('SMTP_USER') === false) {
            error_log("Skipping shipping email: SMTP credentials not set.");
            // We still return success because the status WAS updated
            sendResponse(true, 'Status updated (Email skipped due to missing config).');
        }

        // Recipients
        $fromEmail = get_config('SMTP_FROM_EMAIL') ?: 'orders@example.com';
        $fromName = get_config('SMTP_FROM_NAME') ?: 'GreenFairway Tees';
        $mail->setFrom($fromEmail, $fromName);
        $mail->addAddress($customerEmail, $customerName);

        // Content
        $mail->isHTML(true);
        $mail->Subject = 'Your Order Has Shipped! ⛳';
        $mail->Body    = "
            <div style='font-family: sans-serif; padding: 20px;'>
                <h2 style='color: #16a34a;'>Great News!</h2>
                <p>Hi $customerName,</p>
                <p>Your order <strong>#$id</strong> has been shipped and is on its way to you.</p>
                <p>Get ready to hit the fairways!</p>
                <br>
                <p>Best regards,<br>The Martini Golf Tees Canada Team</p>
            </div>
        ";

        $mail->send();
        error_log("Shipping email sent for order $id");
        sendResponse(true, 'Status updated and email sent.');

    } catch (Exception $e) {
        error_log("Message could not be sent. Mailer Error: {$mail->ErrorInfo}");
        // Return success for the UPDATE, but warn about email
        sendResponse(true, 'Status updated but email failed to send.');
    }
} else {
    sendResponse(true, 'Status updated.');
}
