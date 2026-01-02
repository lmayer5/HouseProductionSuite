<?php
// test_services.php
// Diagnostic script to test SMTP and GitHub connectivity
// Version: 1.1

ini_set('display_errors', 1);
error_reporting(E_ALL);

require 'vendor/autoload.php';
require_once 'env_loader.php';

use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\SMTP;
use PHPMailer\PHPMailer\Exception;

echo "<h1>Service Connectivity Test</h1>";
echo "<a href='index.html'>&larr; Back to Home</a><hr>";

// 1. SMTP TEST
echo "<h2>1. SMTP Email Test</h2>";
$smtpUser = get_config('SMTP_USER');
$smtpPass = get_config('SMTP_PASS');
$smtpHost = get_config('SMTP_HOST');

echo "<b>Config Loaded:</b> " . ($smtpUser ? "Yes ($smtpUser)" : "NO") . "<br>";
echo "<b>Password Loaded:</b> " . ($smtpPass ? "Yes (Length: " . strlen($smtpPass) . ")" : "NO") . "<br>";

if (!$smtpUser || !$smtpPass) {
    echo "<p style='color:red'>Skipping SMTP test: Credentials missing in private_data/config.php</p>";
} else {
    $mail = new PHPMailer(true);
    try {
        // Server settings
        $mail->SMTPDebug = SMTP::DEBUG_CONNECTION; // Enable verbose debug output
        $mail->Debugoutput = 'html'; // Output properly formatted HTML
        $mail->isSMTP();
        $mail->Host       = $smtpHost ?: 'smtp.hostinger.com';
        $mail->SMTPAuth   = true;
        $mail->Username   = $smtpUser;
        $mail->Password   = $smtpPass;
        $mail->SMTPSecure = PHPMailer::ENCRYPTION_STARTTLS;
        $mail->Port       = get_config('SMTP_PORT') ?: 587;
        $mail->Timeout    = 10;

        echo "<div style='background:#f0f0f0; padding:10px; font-family:monospace; border:1px solid #ccc;'>";
        // Connect verify
        if ($mail->smtpConnect()) {
            echo "</div>";
            echo "<h3 style='color:green'>SMTP CONNECTED SUCCESSFULLY!</h3>";
            echo "Credentials are correct.";
            $mail->smtpClose();
        } else {
            echo "</div>";
            echo "<h3 style='color:red'>SMTP CONNECTION FAILED</h3>";
        }
    } catch (Exception $e) {
        echo "</div>";
        echo "<h3 style='color:red'>SMTP Error: " . $e->getMessage() . "</h3>";
    }
}

echo "<hr>";

// 2. GITHUB TEST
echo "<h2>2. GitHub API Test</h2>";

// Load Settings for GitHub
$privateDataDir = realpath(__DIR__ . '/private_data');
if (!$privateDataDir) $privateDataDir = realpath(__DIR__ . '/../private_data');

$settingsFile = $privateDataDir . '/settings.json';
$settings = [];
if (file_exists($settingsFile)) {
    $settings = json_decode(file_get_contents($settingsFile), true) ?? [];
} else {
    echo "Warning: settings.json not found.<br>";
}

$githubToken = !empty($settings['github_pat']) ? $settings['github_pat'] : get_config('GITHUB_PAT');
$githubRepo = !empty($settings['github_repo']) ? $settings['github_repo'] : get_config('GITHUB_REPO');

echo "<b>PAT Loaded:</b> " . ($githubToken ? "Yes (Length: " . strlen($githubToken) . ")" : "NO") . "<br>";
echo "<b>Default Repo:</b> " . ($githubRepo ? $githubRepo : "NO") . "<br>";

if (!$githubToken) {
    echo "<p style='color:red'>Skipping GitHub test: No PAT found in settings.</p>";
} else {
    // Clean repo string
    $targetRepo = $githubRepo;
    $targetRepo = str_replace('https://github.com/', '', $targetRepo);
    $targetRepo = trim($targetRepo, '/');
    
    if (empty($targetRepo)) {
        echo "<p style='color:red'>Skipping Repo test: No Repo configured.</p>";
    } else {
        $url = "https://api.github.com/repos/$targetRepo";
        
        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_HTTPHEADER, [
            'Authorization: Bearer ' . $githubToken,
            'Accept: application/vnd.github.v3+json',
            'User-Agent: Gradient-Solutions-Internal-Test'
        ]);
        
        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);
        
        echo "<div style='background:#f0f0f0; padding:10px; font-family:monospace; border:1px solid #ccc;'>";
        echo "Testing Access to: $targetRepo<br>";
        echo "HTTP Status: $httpCode<br>";
        
        $data = json_decode($response, true);
        if ($httpCode === 200) {
            echo "Name: " . ($data['full_name'] ?? 'Unknown') . "<br>";
            echo "Private: " . ($data['private'] ? 'Yes' : 'No') . "<br>";
            echo "Permissions: " . json_encode($data['permissions'] ?? []) . "<br>";
            echo "</div>";
            echo "<h3 style='color:green'>GITHUB CONNECTED SUCCESSFULLY!</h3>";
        } else {
            echo "Error Message: " . ($data['message'] ?? 'Unknown') . "<br>";
            echo "</div>";
            echo "<h3 style='color:red'>GITHUB FAILED (Code $httpCode)</h3>";
            if ($httpCode === 404) echo "Tip: 404 usually means key is valid but misses 'repo' scope, OR the repo name is wrong.";
            if ($httpCode === 401) echo "Tip: 401 means the Token is invalid/revoked.";
        }
    }
}
?>
