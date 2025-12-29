<?php
// process_order.php
// Handles order submission: saves to JSON and emails invoice.

// 1. Load Composer autoloader
require 'vendor/autoload.php';
require_once 'env_loader.php';

use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\Exception;

header('Content-Type: application/json');

// Helper to send JSON response
function sendResponse($status, $message, $data = []) {
    error_log("Sending Response: $status - $message");
    echo json_encode(['status' => $status, 'message' => $message, 'data' => $data]);
    exit;
}

error_log("Recieved request: " . $_SERVER['REQUEST_METHOD']);


// 2. data Validation
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    sendResponse('error', 'Invalid request method.');
}

$name = $_POST['customer_name'] ?? '';
$email = $_POST['customer_email'] ?? '';
$orderJson = $_POST['order_json'] ?? '';
$pdfFile = $_FILES['invoice_pdf'] ?? null;

if (empty($name) || empty($email) || empty($orderJson) || !$pdfFile) {
    sendResponse('error', 'Missing required fields.');
}

// 3. Save Order to persistent storage (Run sanitization first)
// SECURE PATH: Move out of public_html
$ordersFile = __DIR__ . '/../private_data/orders.json';
$currentOrders = [];

if (file_exists($ordersFile)) {
    $fileContent = file_get_contents($ordersFile);
    $currentOrders = json_decode($fileContent, true) ?? [];
}

$newOrder = [
    'id' => uniqid('ORD-'),
    'timestamp' => date('c'),
    'customer' => [
        'name' => htmlspecialchars($name),
        'email' => htmlspecialchars($email)
    ],
    'details' => json_decode($orderJson, true)
];

$currentOrders[] = $newOrder;

// Save PDF to disk
$invoicesDir = __DIR__ . '/../private_data/invoices';
if (!is_dir($invoicesDir)) {
    mkdir($invoicesDir, 0755, true);
}
$invoiceFilename = $newOrder['id'] . '.pdf';
$invoicePath = $invoicesDir . '/' . $invoiceFilename;

if (move_uploaded_file($pdfFile['tmp_name'], $invoicePath)) {
    // Update order with invoice file path (relative to private_data/invoices for security)
    $currentOrders[count($currentOrders) - 1]['invoice_file'] = $invoiceFilename;
} else {
    error_log("Failed to save invoice PDF to disk.");
}

error_log("Attempting to save order to $ordersFile");
if (file_put_contents($ordersFile, json_encode($currentOrders, JSON_PRETTY_PRINT)) === false) {
    // Log error but might still try to email
    error_log("Failed to write to orders.json");
} else {
    error_log("Order saved successfully.");
}

// 4. Send Email via PHPMailer
error_log("Initializing PHPMailer...");
$mail = new PHPMailer(true);

try {
    // Server settings from environment variables
    $mail->isSMTP();
    $mail->Host       = get_config('SMTP_HOST') ?: 'smtp.hostinger.com';
    $mail->SMTPAuth   = true;
    $mail->Username   = get_config('SMTP_USER') ?: 'orders@example.com';
    $mail->Password   = get_config('SMTP_PASS') ?: 'secret';
    $mail->SMTPSecure = PHPMailer::ENCRYPTION_STARTTLS;
    $mail->Port       = get_config('SMTP_PORT') ?: 587;
    $mail->Timeout    = 5; // Set short timeout (5 seconds) just in case

    // Check for missing SMTP credentials
    if (get_config('SMTP_USER') === false) {
        error_log("Skipping email sending: SMTP credentials not configured.");
        sendResponse('success', 'Order processed (Email skipped - SMTP not configured).');
    }

    // Recipients
    $fromEmail = get_config('SMTP_FROM_EMAIL') ?: 'orders@martinitees.ca';
    $fromName = get_config('SMTP_FROM_NAME') ?: 'Martini Golf Tees Canada';
    $mail->setFrom($fromEmail, $fromName);
    $mail->addAddress($email, $name);     // Add a recipient

    // Attachments
    // Note: process_order.php moved the file, so we use the saved path
    if (file_exists($invoicePath)) {
        $mail->addAttachment($invoicePath, 'Martini_Invoice.pdf');
    } elseif ($pdfFile['error'] === UPLOAD_ERR_OK) {
        // Fallback if move failed (unlikely if logic above worked, but safe)
        $mail->addAttachment($pdfFile['tmp_name'], 'Martini_Invoice.pdf');
    } else {
        throw new Exception("File upload error: " . $pdfFile['error']);
    }

    // Content
    $mail->isHTML(true);
    $mail->Subject = 'Your Martini Golf Tees Invoice';
    $mail->Body    = "
        <h1>Thank you for your order, $name!</h1>
        <p>We have received your order and it is being processed.</p>
        <p>Please find your invoice attached.</p>
        <br>
        <p>Best regards,<br>The Martini Golf Tees Canada Team</p>
    ";
    $mail->AltBody = "Thank you for your order, $name! Please find your invoice attached.";

    $mail->send();
    error_log("Customer email sent successfully.");

    // 5. Send Admin Notification Email
    $adminEmail = getenv('ADMIN_EMAIL') ?: getenv('SMTP_FROM_EMAIL') ?: null;
    
    if ($adminEmail) {
        try {
            $adminMail = new PHPMailer(true);
            $adminMail->isSMTP();
            $adminMail->Host       = getenv('SMTP_HOST') ?: 'smtp.hostinger.com';
            $adminMail->SMTPAuth   = true;
            $adminMail->Username   = getenv('SMTP_USER') ?: 'orders@example.com';
            $adminMail->Password   = getenv('SMTP_PASS') ?: 'secret';
            $adminMail->SMTPSecure = PHPMailer::ENCRYPTION_STARTTLS;
            $adminMail->Port       = getenv('SMTP_PORT') ?: 587;
            $adminMail->Timeout    = 5;

            $adminMail->setFrom($fromEmail, $fromName);
            $adminMail->addAddress($adminEmail);

            // Attach invoice to admin email too
            if (file_exists($invoicePath)) {
                $adminMail->addAttachment($invoicePath, 'Martini_Invoice.pdf');
            }

            // Build order summary for admin
            $orderDetails = json_decode($orderJson, true);
            $orderTotal = $orderDetails['total'] ?? 'N/A';
            $orderId = $newOrder['id'];
            $orderTime = date('M d, Y h:i A');

            $adminMail->isHTML(true);
            $adminMail->Subject = "🛒 New Order Received - $orderId";
            $adminMail->Body = "
                <div style='font-family: sans-serif; padding: 20px;'>
                    <h2 style='color: #16a34a;'>New Order Received!</h2>
                    <p><strong>Order ID:</strong> $orderId</p>
                    <p><strong>Date:</strong> $orderTime</p>
                    <hr style='border: 1px solid #eee;'>
                    <h3>Customer Details</h3>
                    <p><strong>Name:</strong> $name</p>
                    <p><strong>Email:</strong> $email</p>
                    <hr style='border: 1px solid #eee;'>
                    <p><strong>Order Total:</strong> \$$orderTotal CAD</p>
                    <p style='color: #666; font-size: 12px;'>Invoice PDF attached. View full order details in the <a href='admin.php'>Admin Dashboard</a>.</p>
                </div>
            ";
            $adminMail->AltBody = "New Order: $orderId from $name ($email). Total: \$$orderTotal CAD";

            $adminMail->send();
            error_log("Admin notification email sent to $adminEmail");
        } catch (Exception $adminEx) {
            // Log but don't fail the order - customer email already sent
            error_log("Admin notification failed: " . $adminEx->getMessage());
        }
    }

    sendResponse('success', 'Order processed and emails sent.');

} catch (Exception $e) {
    sendResponse('error', "Message could not be sent. Mailer Error: {$mail->ErrorInfo}");
}
