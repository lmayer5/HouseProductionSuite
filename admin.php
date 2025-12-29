<?php
session_start();

// Configuration
require_once 'env_loader.php';

$password = get_config('ADMIN_PASSWORD');
if (!$password) {
    die("Configuration Error: Admin password not set in environment. Please check your Hostinger 'Environment Variables' or create a 'private_data/config.php' file.");
}
// SECURE PATH: Move out of public_html
$ordersFile = __DIR__ . '/../private_data/orders.json';

// Handle Logout
if (isset($_GET['logout'])) {
    session_destroy();
    header('Location: admin.php');
    exit;
}

// Handle Login POST
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['password'])) {
    if ($_POST['password'] === $password) {
        $_SESSION['logged_in'] = true;
        header('Location: admin.php');
        exit;
    } else {
        $error = "Incorrect password.";
    }
}

// Check Authentication
if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    // ... Login Form (omitted for brevity, keep existing) ...
    ?>
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Admin Login - GreenFairway Tees</title>
        <link href="https://cdn.jsdelivr.net/npm/daisyui@4.12.10/dist/full.min.css" rel="stylesheet" type="text/css" />
        <script src="https://cdn.tailwindcss.com"></script>
    </head>
    <body class="bg-gray-100 h-screen flex items-center justify-center">
        <div class="card w-96 bg-base-100 shadow-xl">
            <div class="card-body">
                <h2 class="card-title justify-center mb-4">Admin Login</h2>
                <form method="POST">
                    <div class="form-control w-full max-w-xs">
                        <label class="label">
                            <span class="label-text">Password</span>
                        </label>
                        <input type="password" name="password" placeholder="Enter password" class="input input-bordered w-full max-w-xs" />
                    </div>
                    <?php if (isset($error)): ?>
                        <div class="alert alert-error mt-4 py-2 text-sm">
                            <span><?= htmlspecialchars($error) ?></span>
                        </div>
                    <?php endif; ?>
                    <div class="card-actions justify-end mt-6">
                        <button type="submit" class="btn btn-primary w-full">Login</button>
                    </div>
                </form>
            </div>
        </div>
    </body>
    </html>
    <?php
    exit;
}

// SETTINGS FILE PATH
// Try internal first for local dev, then external for prod
$settingsFile = __DIR__ . '/private_data/settings.json';
if (!file_exists($settingsFile)) {
    $settingsFile = __DIR__ . '/../private_data/settings.json';
}

// HANDLE CLEAR ORDERS
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action']) && $_POST['action'] === 'clear_orders') {
    if (file_put_contents($ordersFile, json_encode([], JSON_PRETTY_PRINT))) {
        $msg = "All orders have been cleared.";
    } else {
        $err = "Failed to clear orders.";
    }
}

// HANDLE SETTINGS UPDATE
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action']) && $_POST['action'] === 'update_settings') {
    // Process FAQs
    $faqs = [];
    if (isset($_POST['faq_questions']) && isset($_POST['faq_answers'])) {
        foreach ($_POST['faq_questions'] as $index => $question) {
            $answer = $_POST['faq_answers'][$index] ?? '';
            if (!empty(trim($question))) {
                $faqs[] = ['question' => $question, 'answer' => $answer];
            }
        }
    }

    $newSettings = [
        'address' => $_POST['address'] ?? '',
        'email' => $_POST['email'] ?? '',
        'phone' => $_POST['phone'] ?? '',
        'timezone' => $_POST['timezone'] ?? 'America/Toronto',
        'about_title' => $_POST['about_title'] ?? 'About Us',
        'about_text' => $_POST['about_text'] ?? '',
        'faq' => $faqs,
        'pricing' => [
            '1' => (float)($_POST['price_1'] ?? 16),
            '2' => (float)($_POST['price_2'] ?? 28),
            '3' => (float)($_POST['price_3'] ?? 38),
            '4' => (float)($_POST['price_4'] ?? 47),
            '5' => (float)($_POST['price_5'] ?? 56),
            '6' => (float)($_POST['price_6'] ?? 66),
            'extra' => (float)($_POST['price_extra'] ?? 11),
        ]
    ];
    
    if (file_put_contents($settingsFile, json_encode($newSettings, JSON_PRETTY_PRINT))) {
        $msg = "Settings updated successfully.";
    } else {
        $err = "Failed to write settings file.";
    }
}

// LOAD SETTINGS
$currentSettings = [
    "address" => "10138 Red Pine Road",
    "email" => "bill@stubberfield.ca",
    "phone" => "519-733-2010",
    "timezone" => "America/Toronto",
    "pricing" => ["1"=>16,"2"=>28,"3"=>38,"4"=>47,"5"=>56,"6"=>66,"extra"=>11]
];
if (file_exists($settingsFile)) {
    $currentSettings = json_decode(file_get_contents($settingsFile), true) ?? $currentSettings;
}

// Set Timezone for Display
date_default_timezone_set($currentSettings['timezone'] ?? 'America/Toronto');


// Read Orders
$orders = [];
if (file_exists($ordersFile)) {
    $content = file_get_contents($ordersFile);
    $orders = json_decode($content, true) ?? [];
    // Sort by date desc
    usort($orders, function($a, $b) {
        return strtotime($b['timestamp']) - strtotime($a['timestamp']);
    });
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard - GreenFairway Tees</title>
    <link href="https://cdn.jsdelivr.net/npm/daisyui@4.12.10/dist/full.min.css" rel="stylesheet" type="text/css" />
    <script src="https://cdn.tailwindcss.com"></script>
    <script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.14.1/dist/cdn.min.js"></script>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
         @import url('https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700&display=swap');
         body { font-family: 'Outfit', sans-serif; }
    </style>
</head>
<body class="bg-gray-50 min-h-screen">

    <!-- Navbar -->
        <div class="navbar bg-base-100 shadow-sm glass-panel sticky top-0 z-50">
            <div class="flex-1">
                <a class="btn btn-ghost text-xl text-brand-800">
                    <i class="fa-solid fa-shield-halved"></i>
                    Martini Admin
                </a>
            </div><a href="?logout=1" class="btn btn-sm btn-ghost text-red-500">Logout</a>
        </div>


    <!-- Content -->
    <div class="container mx-auto p-6 max-w-6xl">
        <!-- Notifications -->
        <?php if (isset($msg)): ?>
            <div class="alert alert-success mb-6"><?= htmlspecialchars($msg) ?></div>
        <?php endif; ?>
        <?php if (isset($err)): ?>
            <div class="alert alert-error mb-6"><?= htmlspecialchars($err) ?></div>
        <?php endif; ?>

        <!-- Settings Section -->
        <div class="bg-white rounded-lg shadow mb-8 p-6" x-data="{ open: false }">
            <button @click="open = !open" class="flex justify-between items-center w-full text-left">
                <h2 class="text-xl font-bold text-gray-800"><i class="fa-solid fa-gear mr-2"></i>Global Settings</h2>
                <i class="fa-solid fa-chevron-down" :class="{'rotate-180': open, 'transition-transform': true}"></i>
            </button>
            
            <div x-show="open" class="mt-4 border-t pt-4" style="display: none;">
                <form method="POST" class="grid grid-cols-1 md:grid-cols-3 gap-4">
                    <input type="hidden" name="action" value="update_settings">
                    
                    <div class="form-control">
                        <label class="label"><span class="label-text">Return Address</span></label>
                        <input type="text" name="address" value="<?= htmlspecialchars($currentSettings['address']) ?>" class="input input-bordered text-sm" required />
                    </div>
                    
                    <div class="form-control">
                        <label class="label"><span class="label-text">Contact Email</span></label>
                        <input type="email" name="email" value="<?= htmlspecialchars($currentSettings['email']) ?>" class="input input-bordered text-sm" required />
                    </div>

                    <div class="form-control">
                        <label class="label"><span class="label-text">Phone Number</span></label>
                        <input type="text" name="phone" value="<?= htmlspecialchars($currentSettings['phone']) ?>" class="input input-bordered text-sm" required />
                    </div>

                    <div class="form-control">
                        <label class="label"><span class="label-text">Timezone</span></label>
                        <select name="timezone" class="select select-bordered text-sm">
                            <option value="America/Toronto" <?= ($currentSettings['timezone'] ?? '') == 'America/Toronto' ? 'selected' : '' ?>>America/Toronto (EST)</option>
                            <option value="America/Vancouver" <?= ($currentSettings['timezone'] ?? '') == 'America/Vancouver' ? 'selected' : '' ?>>America/Vancouver (PST)</option>
                            <option value="UTC" <?= ($currentSettings['timezone'] ?? '') == 'UTC' ? 'selected' : '' ?>>UTC</option>
                        </select>
                    </div>

                    <div class="md:col-span-3 border-t pt-2 mt-2">
                         <h3 class="font-bold text-sm mb-2">About Section</h3>
                         <div class="grid grid-cols-1 gap-4">
                            <div class="form-control">
                                <label class="label"><span class="label-text">Section Title</span></label>
                                <input type="text" name="about_title" value="<?= htmlspecialchars($currentSettings['about_title'] ?? '') ?>" class="input input-bordered text-sm" />
                            </div>
                            <div class="form-control">
                                <label class="label"><span class="label-text">About Text</span></label>
                                <textarea name="about_text" class="textarea textarea-bordered h-24 text-sm"><?= htmlspecialchars($currentSettings['about_text'] ?? '') ?></textarea>
                            </div>
                         </div>
                    </div>

                    <div class="md:col-span-3 border-t pt-2 mt-2" x-data="{ faqs: <?= htmlspecialchars(json_encode($currentSettings['faq'] ?? []), ENT_QUOTES, 'UTF-8') ?> }">
                        <h3 class="font-bold text-sm mb-2">FAQ Management</h3>
                        <div class="space-y-4">
                            <template x-for="(faq, index) in faqs" :key="index">
                                <div class="flex gap-2 items-start bg-gray-50 p-3 rounded-lg border">
                                    <div class="flex-1 grid gap-2">
                                        <input type="text" name="faq_questions[]" x-model="faq.question" placeholder="Question" class="input input-sm input-bordered w-full" required />
                                        <textarea name="faq_answers[]" x-model="faq.answer" placeholder="Answer" class="textarea textarea-sm textarea-bordered w-full" required></textarea>
                                    </div>
                                    <button type="button" @click="faqs.splice(index, 1)" class="btn btn-ghost btn-xs text-red-500 hover:bg-red-50"><i class="fa-solid fa-trash"></i></button>
                                </div>
                            </template>
                        </div>
                        <div class="flex gap-2 mt-2">
                            <button type="button" @click="faqs.push({question: '', answer: ''})" class="btn btn-sm btn-outline btn-success">
                                <i class="fa-solid fa-plus"></i> Add Question
                            </button>
                            <button type="submit" class="btn btn-sm btn-primary">
                                <i class="fa-solid fa-save"></i> Save Changes
                            </button>
                        </div>
                        <!-- Hidden inputs if x-for doesn't submit nicely (it usually doesn't for purely dynamic lists without name attributes, but we added name attributes above) -->
                    </div>

                    <div class="md:col-span-3 border-t pt-2 mt-2">
                        <h3 class="font-bold text-sm mb-2">Bundle Pricing Strategy (Tax & Shipping Included)</h3>
                        <div class="grid grid-cols-2 md:grid-cols-4 gap-2">
                            <div class="form-control">
                                <label class="label p-0"><span class="label-text-alt">1 Pack</span></label>
                                <input type="number" step="0.01" name="price_1" value="<?= htmlspecialchars($currentSettings['pricing']['1'] ?? 16) ?>" class="input input-bordered input-sm" required />
                            </div>
                            <div class="form-control">
                                <label class="label p-0"><span class="label-text-alt">2 Packs</span></label>
                                <input type="number" step="0.01" name="price_2" value="<?= htmlspecialchars($currentSettings['pricing']['2'] ?? 28) ?>" class="input input-bordered input-sm" required />
                            </div>
                            <div class="form-control">
                                <label class="label p-0"><span class="label-text-alt">3 Packs</span></label>
                                <input type="number" step="0.01" name="price_3" value="<?= htmlspecialchars($currentSettings['pricing']['3'] ?? 38) ?>" class="input input-bordered input-sm" required />
                            </div>
                            <div class="form-control">
                                <label class="label p-0"><span class="label-text-alt">4 Packs</span></label>
                                <input type="number" step="0.01" name="price_4" value="<?= htmlspecialchars($currentSettings['pricing']['4'] ?? 47) ?>" class="input input-bordered input-sm" required />
                            </div>
                            <div class="form-control">
                                <label class="label p-0"><span class="label-text-alt">5 Packs</span></label>
                                <input type="number" step="0.01" name="price_5" value="<?= htmlspecialchars($currentSettings['pricing']['5'] ?? 56) ?>" class="input input-bordered input-sm" required />
                            </div>
                            <div class="form-control">
                                <label class="label p-0"><span class="label-text-alt">6 Packs</span></label>
                                <input type="number" step="0.01" name="price_6" value="<?= htmlspecialchars($currentSettings['pricing']['6'] ?? 66) ?>" class="input input-bordered input-sm" required />
                            </div>
                            <div class="form-control">
                                <label class="label p-0"><span class="label-text-alt">Per Pack > 6</span></label>
                                <input type="number" step="0.01" name="price_extra" value="<?= htmlspecialchars($currentSettings['pricing']['extra'] ?? 11) ?>" class="input input-bordered input-sm" required />
                            </div>
                        </div>
                    </div>

                    <div class="md:col-span-3 text-right mt-2">
                        <button type="submit" class="btn btn-primary btn-sm">Save Global Settings</button>
                    </div>
                </form>
            </div>
        </div>

        <div class="flex justify-between items-center mb-6">
            <h1 class="text-2xl font-bold text-gray-800">Recent Orders</h1>
            <div class="flex items-center gap-4">
                <form method="POST" onsubmit="return confirm('Are you sure you want to delete ALL orders? This cannot be undone.');">
                    <input type="hidden" name="action" value="clear_orders">
                    <button type="submit" class="btn btn-sm btn-error text-white">
                        <i class="fa-solid fa-trash-can mr-2"></i>Clear All Orders
                    </button>
                </form>
                <div class="text-sm text-gray-500">Total Orders: <?= count($orders) ?></div>
            </div>
        </div>

        <div class="overflow-x-auto bg-white rounded-lg shadow">
            <table class="table w-full">
                <thead>
                    <tr class="bg-gray-100 text-gray-600 uppercase text-xs leading-normal">
                        <th>Date</th>
                        <th>Order ID</th>
                        <th>Customer</th>
                        <th>Status</th>
                        <th>Total</th>
                        <th class="text-right">Actions</th>
                    </tr>
                </thead>
                <tbody class="text-gray-600 text-sm font-light">
                    <?php if (empty($orders)): ?>
                        <tr>
                            <td colspan="6" class="text-center py-8 text-gray-400">No orders found.</td>
                        </tr>
                    <?php else: ?>
                        <?php foreach ($orders as $order): ?>
                            <?php 
                                $status = $order['status'] ?? 'Pending'; 
                                $total = $order['details']['total'] ?? $order['total'] ?? 0;
                            ?>
                            <tr class="border-b border-gray-200 hover:bg-gray-50 transition-colors" x-data="{ 
                                status: '<?= $status ?>',
                                isLoading: false,
                                markShipped() {
                                    if(!confirm('Mark this order as Shipped?')) return;
                                    this.isLoading = true;
                                    
                                    fetch('update_status.php', {
                                        method: 'POST',
                                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                                        body: 'id=<?= $order['id'] ?>&status=Shipped'
                                    })
                                    .then(res => res.json())
                                    .then(data => {
                                        if(data.success) {
                                            this.status = 'Shipped';
                                        } else {
                                            alert('Error: ' + (data.message || 'Unknown error'));
                                        }
                                    })
                                    .catch(err => alert('Network Error'))
                                    .finally(() => this.isLoading = false);
                                }
                            }">
                                <td class="py-3 px-6 whitespace-nowrap">
                                    <?= date('M d, Y h:i A', strtotime($order['timestamp'])) ?>
                                </td>
                                <td class="py-3 px-6 font-mono text-xs">
                                    <?= $order['id'] ?>
                                </td>
                                <td class="py-3 px-6">
                                    <div class="flex items-center">
                                        <div>
                                            <div class="font-medium text-gray-800"><?= htmlspecialchars($order['customer']['name']) ?></div>
                                            <div class="text-xs text-gray-400"><?= htmlspecialchars($order['customer']['email']) ?></div>
                                        </div>
                                    </div>
                                </td>
                                <td class="py-3 px-6">
                                    <template x-if="status === 'Pending'">
                                        <span class="badge badge-warning gap-2">
                                            Pending
                                        </span>
                                    </template>
                                    <template x-if="status === 'Shipped'">
                                        <span class="badge badge-success text-white gap-2">
                                            Shipped
                                        </span>
                                    </template>
                                </td>
                                <td class="py-3 px-6">
                                    $<?= number_format((float)$total, 2) ?>
                                </td>
                                <td class="py-3 px-6 text-right">
                                    <a 
                                        x-show="status === 'Shipped' || true" 
                                        href="<?= isset($order['invoice_file']) ? 'view_invoice.php?file=' . htmlspecialchars($order['invoice_file']) : '#' ?>" 
                                        target="_blank"
                                        class="btn btn-xs btn-outline btn-info mr-2"
                                        :class="{ 'btn-disabled': !'<?= isset($order['invoice_file']) ? $order['invoice_file'] : '' ?>' }"
                                    >
                                        <i class="fa-solid fa-file-pdf"></i> PDF
                                    </a>
                                    <button 
                                        x-show="status === 'Pending'"
                                        @click="markShipped()" 
                                        :disabled="isLoading"
                                        class="btn btn-xs btn-outline btn-success"
                                    >
                                        <span x-show="isLoading" class="loading loading-spinner loading-xs"></span>
                                        Mark Shipped
                                    </button>
                                </td>
                            </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
    </div>

</body>
</html>
