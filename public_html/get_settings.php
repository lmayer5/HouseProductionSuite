<?php
// get_settings.php
header('Content-Type: application/json');

// Path to settings file
// Using internal path strictly for local compat, ensure deployment guide mentions moving it if consistent structure desired
// For now, consistent with where I wrote the file:
$settingsFile = __DIR__ . '/private_data/settings.json';

// Fallback logic for production if moved outside
if (!file_exists($settingsFile)) {
    $settingsFile = __DIR__ . '/../private_data/settings.json';
}

if (file_exists($settingsFile)) {
    echo file_get_contents($settingsFile);
} else {
    // Default fallback if file missing
    echo json_encode([
        "address" => "123 tech avenue, toronto, on",
        "email" => "hello@gradientsound.shop",
        "phone" => "416-555-0199",
        "timezone" => "America/Toronto",
        "about_title" => "about gradient sound",
        "about_text" => "gradient sound is a boutique audio technology studio owned and operated by luke mayer, our chief audio engineer. we focus on creating high-performance vst3 and au plugins with clinical precision and musical character. as a sole proprietorship, we provide a direct and personal connection to the tools you use. payments are processed securely via e-transfer; once received, you will be invited to a private github repository for instant digital delivery of your plugins.",
        "faq" => [
            ["question" => "how do i get my plugins?", "answer" => "after your e-transfer payment is confirmed, you will receive an automated invitation to a private github repository containing your downloads."],
            ["question" => "which daws are supported?", "answer" => "our plugins are compatible with major daws supporting vst3 or au formats, including ableton live, fl studio, and logic pro."],
            ["question" => "do you offer refunds?", "answer" => "due to the digital nature of our products, all sales are final. we encourage you to try our free rhythm engine before purchasing."]
        ],
        "pricing" => [
            "individual" => 49
        ]
    ]);
}
exit;
