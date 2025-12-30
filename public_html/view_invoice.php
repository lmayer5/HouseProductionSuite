<?php
// Enable error reporting for debugging
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

session_start();

// Configuration
$invoicesDir = __DIR__ . '/../private_data/invoices';

// Check Authentication
if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    http_response_code(403);
    die('Unauthorized');
}

// Get File Parameter
$file = $_GET['file'] ?? '';

// Basic Sanitization (prevent directory traversal)
$file = basename($file); 

// Full Path
$filePath = $invoicesDir . '/' . $file;

// Verify File Exists
if (!$file || !file_exists($filePath)) {
    http_response_code(404);
    die('Invoice not found.');
}

// Serve PDF
header('Content-Type: application/pdf');
header('Content-Disposition: inline; filename="' . $file . '"');
header('Content-Length: ' . filesize($filePath));

// Clear output buffer to avoid corrupting PDF
if (ob_get_length()) ob_clean();
flush();

readfile($filePath);
exit;
