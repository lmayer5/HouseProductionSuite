<?php
// status_check.php
// Version: 1.0
// Access: Public (Read-only status)

ini_set('display_errors', 1);
error_reporting(E_ALL);

function check($cond) {
    return $cond ? '<span style="color:green; font-weight:bold;">PASS</span>' : '<span style="color:red; font-weight:bold;">FAIL</span>';
}

echo "<h1>System Health Check</h1>";
echo "<p>Use this info to debug '500 Server Errors'.</p>";

echo "<h3>1. PHP Environment</h3>";
echo "PHP Version: " . phpversion() . " <br>";
echo "JSON Enabled: " . check(function_exists('json_encode')) . "<br>";
echo "OpenSSL Enabled: " . check(extension_loaded('openssl')) . "<br>";
echo "File Uploads Enabled: " . check(ini_get('file_uploads')) . "<br>";

echo "<h3>2. File System Structure</h3>";

// Check Vendor (Libraries)
$vendorPath = __DIR__ . '/vendor/autoload.php';
echo "Vendor Libraries (autoload.php): " . $vendorPath . " ... " . check(file_exists($vendorPath));
if (!file_exists($vendorPath)) {
    echo " <br><b>CRITICAL FIX:</b> You must run 'composer install' on the server via SSH or upload the 'vendor' folder manually via FTP.";
}
echo "<br>";

// Check Private Data
$privateDataDir = __DIR__ . '/../private_data';
echo "Private Data Dir: " . $privateDataDir . " ... " . check(is_dir($privateDataDir));
if (is_dir($privateDataDir)) {
    echo " <br>&nbsp;&nbsp;&nbsp;Writable: " . check(is_writable($privateDataDir));
    
    // Check Config
    $configFile = $privateDataDir . '/config.php';
    echo "<br>&nbsp;&nbsp;&nbsp;Config File: " . check(file_exists($configFile));
    
    // Check Orders
    $ordersFile = $privateDataDir . '/orders.json';
    echo "<br>&nbsp;&nbsp;&nbsp;Orders File: " . check(file_exists($ordersFile));
    if (file_exists($ordersFile)) {
        echo " (Writable: " . check(is_writable($ordersFile)) . ")";
    }
} else {
    echo " <br><b>CRITICAL FIX:</b> Use File Manager or SSH to create a folder named 'private_data' one level above public_html.";
}

echo "<h3>3. Test Results</h3>";
if (file_exists($vendorPath) && is_dir($privateDataDir) && is_writable($privateDataDir) && extension_loaded('openssl')) {
    echo "<div style='background: #e6fffa; padding: 20px; border: 2px solid green;'>SYSTEM HEALTHY: READY TO PROCESS ORDERS</div>";
} else {
    echo "<div style='background: #fff5f5; padding: 20px; border: 2px solid red;'>SYSTEM UNHEALTHY: ORDERS WILL FAIL</div>";
}
?>
