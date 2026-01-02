<?php
// process_order.php
// Handles order submission: saves to JSON and emails invoice.

// 0. STRICT OUTPUT BUFFERING
// Catch any stray whitespace, warnings, or errors to ensure clean JSON output
ob_start();

// NUCLEAR ERROR HANDLING: specific catch for fatal startup errors (e.g. missing files)
register_shutdown_function(function() {
    $error = error_get_last();
    if ($error && ($error['type'] === E_ERROR || $error['type'] === E_PARSE || $error['type'] === E_CORE_ERROR || $error['type'] === E_COMPILE_ERROR)) {
        // Clear any half-written output
        if (ob_get_length()) ob_clean(); 
        
        // Force JSON response
        header('Content-Type: application/json');
        http_response_code(200); // Return 200 so frontend parses the JSON
        echo json_encode(['status' => 'error', 'message' => "Critical Server Error: " . $error['message'] . " in " . $error['file'] . " on line " . $error['line']]);
        exit;
    }
});

// Enable error logging but DISABLE display
ini_set('display_errors', 0);
ini_set('log_errors', 1);
error_reporting(E_ALL);

require 'vendor/autoload.php';
require_once 'env_loader.php';

use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\Exception;

// Default headers
header('Content-Type: application/json');

// Helper to send JSON response cleanly
function sendResponse($status, $message, $data = []) {
    // Clear any previous output (warnings, HTML errors)
    ob_clean(); 
    
    echo json_encode(['status' => $status, 'message' => $message, 'data' => $data]);
    
    // Flush buffer and exit
    ob_end_flush();
    exit;
}

try {
    // 1. Data Validation
    if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
        throw new Exception('Invalid request method.');
    }

    $name = trim($_POST['customer_name'] ?? '');
    $email = trim($_POST['customer_email'] ?? '');
    $email = filter_var($email, FILTER_SANITIZE_EMAIL); // Remove illegal characters
    $githubUsername = trim($_POST['github_username'] ?? '');
    $orderJson = $_POST['order_json'] ?? '';
    $pdfFile = $_FILES['invoice_pdf'] ?? null;

    if (empty($name) || empty($email) || empty($orderJson) || !$pdfFile) {
        throw new Exception('Missing required fields.');
    }

    // 2. Save Order to persistent storage
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
            'email' => htmlspecialchars($email),
            'github' => htmlspecialchars($githubUsername)
        ],
        'details' => json_decode($orderJson, true)
    ];

    $currentOrders[] = $newOrder;

    // Save PDF
    $invoicesDir = __DIR__ . '/../private_data/invoices';
    if (!is_dir($invoicesDir)) {
        if (!mkdir($invoicesDir, 0755, true)) {
            error_log("Failed to create invoices directory: $invoicesDir");
        }
    }
    
    $invoiceFilename = $newOrder['id'] . '.pdf';
    $invoicePath = $invoicesDir . '/' . $invoiceFilename;
    $invoiceSaved = false;

    if (move_uploaded_file($pdfFile['tmp_name'], $invoicePath)) {
        $currentOrders[count($currentOrders) - 1]['invoice_file'] = $invoiceFilename;
        $invoiceSaved = true;
    } else {
        error_log("Failed to save invoice PDF to disk.");
    }

    // Save Order JSON (Safely)
    if (empty($currentOrders) && file_exists($ordersFile) && filesize($ordersFile) > 10) {
         error_log("CRITICAL: Attempted to save empty orders list over existing data. Aborted.");
         throw new Exception("Internal storage error. Please contact support.");
    }
    
    $jsonOutput = json_encode($currentOrders, JSON_PRETTY_PRINT);
    if ($jsonOutput === false) {
        throw new Exception("JSON encoding failed: " . json_last_error_msg());
    }

    if (file_put_contents($ordersFile, $jsonOutput) === false) {
         error_log("Failed to write to orders.json");
         $emailWarning = ($emailWarning ?? "") . " ERROR: Could not save order to database.";
    } else {
        // Debug Log
        file_put_contents(__DIR__ . '/../private_data/debug.log', date('c') . " - SAVED: " . $newOrder['id'] . "\n", FILE_APPEND);
    }

    // 3. Send Emails (The risky part)
    $emailWarning = null;
    
    try {
        if (get_config('SMTP_USER') === false) {
            $emailWarning = "SMTP not configured.";
        } else {
            $mail = new PHPMailer(true);
            $mail->isSMTP();
            $mail->Host       = get_config('SMTP_HOST') ?: 'smtp.hostinger.com';
            $mail->SMTPAuth   = true;
            $mail->Username   = get_config('SMTP_USER') ?: 'orders@example.com';
            $mail->Password   = get_config('SMTP_PASS') ?: 'secret';
            $mail->SMTPSecure = PHPMailer::ENCRYPTION_STARTTLS;
            $mail->Port       = get_config('SMTP_PORT') ?: 587;
            $mail->Timeout    = 8;

            $fromEmail = get_config('SMTP_FROM_EMAIL') ?: 'admin@gradientsound.shop';
            $fromName = get_config('SMTP_FROM_NAME') ?: 'Gradient Sound';
            $mail->setFrom($fromEmail, $fromName);
            $mail->addAddress($email, $name);

            if ($invoiceSaved && file_exists($invoicePath)) {
                $mail->addAttachment($invoicePath, 'Gradient_Invoice.pdf');
            } elseif ($pdfFile && isset($pdfFile['tmp_name']) && file_exists($pdfFile['tmp_name'])) {
                 $mail->addAttachment($pdfFile['tmp_name'], 'Gradient_Invoice.pdf');
            }

                $hasFreePlugins = false;
                if (isset($newOrder['details']['cart']) && is_array($newOrder['details']['cart'])) {
                    foreach ($newOrder['details']['cart'] as $item) {
                        // Check for Rhythm Engine (p_rhythm_bass) or Melodic Engine (p_melodic)
                        // IDs might have suffixes like _VST3, so use strpos or check start
                        if (isset($item['id']) && (strpos($item['id'], 'p_rhythm_bass') !== false || strpos($item['id'], 'p_melodic') !== false)) {
                            $hasFreePlugins = true;
                            break;
                        }
                    }
                }

                $downloadLinkHtml = "";
                if ($hasFreePlugins) {
                    $downloadLinkHtml = "
                        <div style='background-color: #f0fdf4; border: 1px solid #22c55e; border-radius: 8px; padding: 15px; margin: 20px 0;'>
                            <h3 style='margin-top: 0; color: #15803d;'>Download Your Plugins</h3>
                            <p style='margin-bottom: 10px;'>You can download the latest release of the House Production Suite (V1.0) directly from GitHub:</p>
                            <a href='https://github.com/lmayer5/HouseProductionSuite/tree/V1.0' style='display: inline-block; background-color: #16a34a; color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px; font-weight: bold;'>Download Release</a>
                        </div>
                    ";
                }

                $mail->isHTML(true);
                $mail->Subject = 'Your Gradient Sound Invoice';
                $mail->Body    = "
                <h1>Thank you for your order, $name!</h1>
                <p>We have received your order and it is being processed.</p>
                " . ($githubUsername ? "<p>An admin will invite your GitHub account (<strong>$githubUsername</strong>) to the private repository shortly.</p>" : "") . "
                $downloadLinkHtml
                <p>Please find your invoice attached.</p>
                <br>
                <p>Best regards,<br>The Gradient Sound Team</p>
            ";

            $mail->send();
            
            // Try Admin Email
            $adminEmail = get_config('ADMIN_EMAIL');
            if ($adminEmail) {
                // ... (Simplified Admin Email Logic for robustness) ...
                $mail->clearAddresses();
                $mail->clearAttachments(); // Clear previous attachments
                $mail->addAddress($adminEmail);
                if ($invoiceSaved && file_exists($invoicePath)) {
                    $mail->addAttachment($invoicePath, 'Gradient_Invoice.pdf');
                }
                $mail->Subject = "New Order: " . $newOrder['id'];
                $mail->Body = "New order received from $name ($email). Total: " . ($newOrder['details']['total'] ?? 'N/A');
                $mail->send();
            }
        }

    } catch (Throwable $emailEx) {
        error_log("Email Error: " . $emailEx->getMessage());
        $emailWarning = "Email failed: " . $emailEx->getMessage();
    }

    $finalMsg = "Order processed successfully.";
    if ($emailWarning) {
        $finalMsg .= " (" . $emailWarning . ")";
    }

    sendResponse('success', $finalMsg);

} catch (Throwable $e) {
    error_log("Global Process Error: " . $e->getMessage());
    // Even global errors get sent as JSON
    sendResponse('error', "Processing Error: " . $e->getMessage());
}
