<?php
// env_loader.php

// Helper function to retrieving configuration values
// Tries multiple sources in order:
// 1. getenv()
// 2. $_SERVER
// 3. $_ENV
// 4. private_data/config.php (fallback file)

function get_config($key) {
    // 1. Try standard environment variable
    $val = getenv($key);
    if ($val !== false && $val !== '') {
        return $val;
    }

    // 2. Try $_SERVER (often populated by web servers)
    if (isset($_SERVER[$key]) && $_SERVER[$key] !== '') {
        return $_SERVER[$key];
    }

    // 3. Try $_ENV (if populated)
    if (isset($_ENV[$key]) && $_ENV[$key] !== '') {
        return $_ENV[$key];
    }

    // 4. Try loading from a local config file (fallback)
    // We cache this so we don't read the file every time
    static $fileConfig = null;
    if ($fileConfig === null) {
        $configPath = __DIR__ . '/private_data/config.php';
        
        // Hostinger structure check: private_data might be a sibling folder
        if (!file_exists($configPath)) {
            $configPath = __DIR__ . '/../private_data/config.php';
        }

        if (file_exists($configPath)) {
            $fileConfig = include($configPath);
        } else {
            $fileConfig = [];
        }
    }

    return $fileConfig[$key] ?? null;
}
