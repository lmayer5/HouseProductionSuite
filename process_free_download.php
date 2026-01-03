<?php
// process_free_download.php
// Lightweight endpoint for free plugin downloads - saves user info to orders.json

// Output buffering for clean JSON
ob_start();

// Error handling
ini_set('display_errors', 0);
ini_set('log_errors', 1);
error_reporting(E_ALL);

header('Content-Type: application/json');

// Helper to send JSON response
function sendResponse($status, $message, $data = []) {
    ob_clean();
    echo json_encode(['status' => $status, 'message' => $message, 'data' => $data]);
    ob_end_flush();
    exit;
}

try {
    // 1. Validate request method
    if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
        throw new Exception('Invalid request method.');
    }

    // 2. Get and validate input
    $name = trim($_POST['name'] ?? '');
    $email = trim($_POST['email'] ?? '');
    $email = filter_var($email, FILTER_SANITIZE_EMAIL);
    $productId = trim($_POST['product_id'] ?? '');
    $productName = trim($_POST['product_name'] ?? '');
    $format = trim($_POST['format'] ?? 'VST3');
    $downloadPath = trim($_POST['download_path'] ?? '');

    if (empty($name) || empty($email) || empty($productId)) {
        throw new Exception('Name, email, and product are required.');
    }

    if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
        throw new Exception('Invalid email address.');
    }

    // 3. Build download record
    $privateDataDir = __DIR__ . '/../private_data';
    if (!is_dir($privateDataDir)) {
        $privateDataDir = __DIR__ . '/private_data';
    }
    
    $ordersFile = $privateDataDir . '/orders.json';
    $currentOrders = [];

    if (file_exists($ordersFile)) {
        $fileContent = file_get_contents($ordersFile);
        $currentOrders = json_decode($fileContent, true) ?? [];
    }

    $newDownload = [
        'id' => uniqid('DL-'),
        'type' => 'free_download',
        'timestamp' => date('c'),
        'customer' => [
            'name' => htmlspecialchars($name),
            'email' => htmlspecialchars($email)
        ],
        'details' => [
            'product_id' => $productId,
            'product_name' => $productName,
            'format' => $format,
            'total' => 0
        ],
        'status' => 'completed'
    ];

    $currentOrders[] = $newDownload;

    // 4. Save to orders.json
    $jsonOutput = json_encode($currentOrders, JSON_PRETTY_PRINT);
    if ($jsonOutput === false) {
        throw new Exception("JSON encoding failed: " . json_last_error_msg());
    }

    if (file_put_contents($ordersFile, $jsonOutput) === false) {
        error_log("Failed to write to orders.json for free download");
        // Continue anyway - download should still work
    }

    // 5. Return success with download URL
    sendResponse('success', 'Download registered successfully.', [
        'download_url' => $downloadPath,
        'download_id' => $newDownload['id']
    ]);

} catch (Throwable $e) {
    error_log("Free Download Error: " . $e->getMessage());
    sendResponse('error', $e->getMessage());
}
