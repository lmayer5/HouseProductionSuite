<?php
// update_status.php
// Updates order status and optionally sends email notification

// 0. STRICT OUTPUT BUFFERING - Prevent any stray output from breaking JSON
ob_start();
ini_set('display_errors', 0);
ini_set('log_errors', 1);
error_reporting(E_ALL);

require 'vendor/autoload.php';
require_once 'env_loader.php';

use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\Exception;

header('Content-Type: application/json');

function sendResponse($success, $message) {
    ob_clean(); // Clear any previous output
    echo json_encode(['success' => $success, 'message' => $message]);
    ob_end_flush();
    exit;
}

try {
    // 1. Validate Input
    $id = $_POST['id'] ?? null;
    $newStatus = $_POST['status'] ?? null;

    if (!$id || !$newStatus) {
        throw new Exception('Missing required fields.');
    }

    // 2. Read Orders
    $ordersFile = __DIR__ . '/../private_data/orders.json';
    if (!file_exists($ordersFile)) {
        throw new Exception('Order database not found.');
    }

    $content = file_get_contents($ordersFile);
    $orders = json_decode($content, true);

    if (!is_array($orders)) {
        throw new Exception('Order database invalid.');
    }

    // 3. Find and Update Order
    $orderIndex = null;
    $customerEmail = null;
    $customerName = null;

    foreach ($orders as $key => $order) {
        if ($order['id'] === $id) {
            $orderIndex = $key;
            $customerEmail = $order['customer']['email'] ?? '';
            $customerName = $order['customer']['name'] ?? '';
            break;
        }
    }

    if ($orderIndex === null) {
        throw new Exception('Order not found.');
    }

    // Update Status
    $orders[$orderIndex]['status'] = $newStatus;
    $orders[$orderIndex]['updated_at'] = date('c');

    // Save to File
    if (file_put_contents($ordersFile, json_encode($orders, JSON_PRETTY_PRINT)) === false) {
        throw new Exception('Failed to save database.');
    }

    // 4. Send Email Notification (Best effort, don't fail if this breaks)
    $emailWarning = null;
    if ($newStatus === 'Completed' && get_config('SMTP_USER') !== false) {
        try {
            $mail = new PHPMailer(true);
            $mail->isSMTP();
            $mail->Host       = get_config('SMTP_HOST') ?: 'smtp.hostinger.com';
            $mail->SMTPAuth   = true;
            $mail->Username   = get_config('SMTP_USER') ?: 'user@example.com';
            $mail->Password   = get_config('SMTP_PASS') ?: 'secret';
            $mail->SMTPSecure = PHPMailer::ENCRYPTION_STARTTLS;
            $mail->Port       = get_config('SMTP_PORT') ?: 587;
            $mail->Timeout    = 8;

            $fromEmail = get_config('SMTP_FROM_EMAIL') ?: 'admin@gradientsound.shop';
            $fromName = get_config('SMTP_FROM_NAME') ?: 'Gradient Sound';
            $mail->setFrom($fromEmail, $fromName);
            $mail->addAddress($customerEmail, $customerName);

            $mail->isHTML(true);
            $mail->Subject = 'Your Order is Complete! 🎹';
            $mail->Body    = "
                <div style='font-family: sans-serif; padding: 20px;'>
                    <h2 style='color: #4f46e5;'>Great News!</h2>
                    <p>Hi $customerName,</p>
                    <p>Your order <strong>#$id</strong> has been completed.</p>
                    <p>Time to make some noise!</p>
                    <br>
                    <p>Best regards,<br>The Gradient Sound Team</p>
                </div>
            ";
            $mail->send();
        } catch (Exception $emailEx) {
            error_log("Status email failed: " . $emailEx->getMessage());
            $emailWarning = " (Email failed)";
        }
    }

    sendResponse(true, 'Status updated.' . ($emailWarning ?? ''));

} catch (Exception $e) {
    error_log("update_status.php error: " . $e->getMessage());
    sendResponse(false, $e->getMessage());
}
