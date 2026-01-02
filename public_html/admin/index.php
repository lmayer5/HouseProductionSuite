<?php
// Enable Error Reporting for Debugging
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

session_start();

// Configuration - Use __DIR__ for robustness
$envLoader = __DIR__ . '/../env_loader.php';
if (!file_exists($envLoader)) {
    die("Error: env_loader.php not found at: " . $envLoader);
}
require_once $envLoader;

$password = get_config('ADMIN_PASSWORD');
if (!$password) {
    die("Configuration Error: ADMIN_PASSWORD not set. Check private_data/config.php.");
}

// DATA PATHS - Use strict paths confirmed by debug.php
// Debug result said: /home/.../private_data/orders.json
$privateDataDir = realpath(__DIR__ . '/../../private_data');
if (!$privateDataDir) {
    // Fallback if realpath fails (e.g. perms), though debug said it's there
    $privateDataDir = __DIR__ . '/../../private_data';
}

$ordersFile = $privateDataDir . '/orders.json';
$settingsFile = $privateDataDir . '/settings.json';

// Handle Logout
if (isset($_GET['logout'])) {
    session_destroy();
    header('Location: index.php');
    exit;
}

// Handle Login POST
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['password'])) {
    if ($_POST['password'] === $password) {
        $_SESSION['logged_in'] = true;
        header('Location: index.php');
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
        <title>Admin Login - Gradient Sound</title>
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

/*
// SETTINGS FILE PATH - REDUNDANT
// $settingsFile = __DIR__ . '/private_data/settings.json';
// if (!file_exists($settingsFile)) {
//     $settingsFile = __DIR__ . '/../private_data/settings.json';
//     if (!file_exists($settingsFile)) {
//         $settingsFile = __DIR__ . '/../../private_data/settings.json';
//     }
// }
*/

// HANDLE CLEAR ORDERS
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action']) && $_POST['action'] === 'clear_orders_confirmed') {
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
        'github_pat' => $_POST['github_pat'] ?? '',
        'github_repo' => $_POST['github_repo'] ?? '',
        'repo_mappings' => json_decode($_POST['repo_mappings_json'] ?? '[]', true),
        'faq' => $faqs
    ];
    
    if (file_put_contents($settingsFile, json_encode($newSettings, JSON_PRETTY_PRINT))) {
        $msg = "Settings updated successfully.";
    } else {
        $err = "Failed to write settings file.";
    }
}

// LOAD SETTINGS
$currentSettings = [
    "address" => "123 audio lane, toronto, on m5v 2h1",
    "email" => "hello@gradientsound.shop",
    "phone" => "416-555-0199",
    "timezone" => "America/Toronto",
    "about_title" => "about gradient sound",
    "about_text" => "gradient sound is a boutique audio technology studio owned and operated by luke mayer, our chief audio engineer. as a sole proprietorship, we provide direct and personal connection to the tools you use. payments are processed via e-transfer, followed by automated github delivery.",
    "faq" => [
        ["question" => "how do i get my plugins?", "answer" => "after your e-transfer payment is confirmed, you will receive an automated invitation to a private github repository containing your downloads."]
    ]
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
    <title>Dashboard - Gradient Sound</title>
    <link href="https://cdn.jsdelivr.net/npm/daisyui@4.12.10/dist/full.min.css" rel="stylesheet" type="text/css" />
    <script src="https://cdn.tailwindcss.com"></script>
    <script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.14.1/dist/cdn.min.js"></script>
    <script>
        tailwind.config = {
            theme: {
                extend: {
                    fontFamily: {
                        sans: ['Outfit', 'sans-serif'],
                    },
                    colors: {
                        brand: {
                            50: '#eff6ff',
                             100: '#dbeafe',
                             200: '#bfdbfe',
                             300: '#93c5fd',
                             400: '#60a5fa',
                             500: '#3b82f6', // Primary Blue
                             600: '#2563eb',
                             800: '#1e40af',
                             900: '#1e3a8a',
                        },
                         secondary: {
                             50: '#faf5ff',
                             500: '#a855f7', // Secondary Purple
                             900: '#581c87',
                        }
                    }
                }
            }
        }
    </script>
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
                    <i class="fa-solid fa-wave-square"></i>
                    Gradient Sound Admin
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
                    
                    <div class="md:col-span-3 border-t pt-2 mt-4">
                         <h3 class="font-bold text-sm mb-2 text-brand-700"><i class="fa-brands fa-github mr-2"></i>GitHub Delivery Settings</h3>
                         <div class="grid grid-cols-1 md:grid-cols-2 gap-4 bg-gray-50 p-4 rounded-lg">
                            <div class="form-control">
                                <label class="label"><span class="label-text">GitHub Personal Access Token (PAT)</span></label>
                                <input type="password" name="github_pat" value="<?= htmlspecialchars($currentSettings['github_pat'] ?? '') ?>" class="input input-bordered text-sm" placeholder="ghp_xxx" />
                            </div>
                            <div class="form-control">
                                <label class="label"><span class="label-text">Default Repository (fallback)</span></label>
                                <input type="text" name="github_repo" value="<?= htmlspecialchars($currentSettings['github_repo'] ?? '') ?>" class="input input-bordered text-sm" placeholder="username/repository" />
                            </div>
                         </div>
                         
                         <div class="mt-4 bg-gray-50 p-4 rounded-lg border border-dashed border-gray-300" x-data="{ mappings: <?= htmlspecialchars(json_encode($currentSettings['repo_mappings'] ?? []), ENT_QUOTES, 'UTF-8') ?> }">
                             <h4 class="text-xs font-bold uppercase text-gray-500 mb-3 tracking-wider">Product-to-Repo Mapping</h4>
                             <p class="text-[10px] text-gray-400 mb-3">Map specific Product IDs (found in catalog) to repositories. If a product isn't mapped, the default repo above is used.</p>
                             <div class="space-y-2">
                                 <template x-for="(map, index) in mappings" :key="index">
                                     <div class="flex gap-2 items-center">
                                         <input type="text" x-model="map.product_id" placeholder="Product ID (e.g. p_stem_gen)" class="input input-xs input-bordered flex-1" />
                                         <i class="fa-solid fa-arrow-right text-gray-300"></i>
                                         <input type="text" x-model="map.repo" placeholder="Repo (e.g. owner/repo)" class="input input-xs input-bordered flex-1" />
                                         <button type="button" @click="mappings.splice(index, 1)" class="btn btn-ghost btn-xs text-red-500"><i class="fa-solid fa-xmark"></i></button>
                                     </div>
                                 </template>
                             </div>
                             <div class="mt-3">
                                 <button type="button" @click="mappings.push({product_id: '', repo: ''})" class="btn btn-xs btn-outline btn-success">
                                     <i class="fa-solid fa-plus"></i> Add Mapping
                                 </button>
                             </div>
                             <input type="hidden" name="repo_mappings_json" :value="JSON.stringify(mappings)">
                         </div>
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

                    <div class="md:col-span-3 text-right mt-6">
                         <button type="submit" class="btn btn-primary">
                             <i class="fa-solid fa-save mr-2"></i> Save Global Settings
                         </button>
                    </div>

                    <!-- Legacy Pricing Section Removed (Pricing is now per-product in Storefront) -->
                    <!-- 
                    <div class="md:col-span-3 border-t pt-2 mt-2">
                        <h3 class="font-bold text-sm mb-2">Bundle Pricing Strategy (Tax & Shipping Included)</h3>
                         ...
                    </div> 
                    -->
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
                    <input type="hidden" name="action" value="clear_orders_confirmed">
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
                                $github = $order['customer']['github'] ?? '';
                            ?>
                            <tr class="border-b border-gray-200 hover:bg-gray-50 transition-colors" x-data="{ 
                                status: '<?= $status ?>',
                                github: '<?= $github ?>',
                                isLoading: false,
                                isInviting: false,
                                markShipped() {
                                    if(!confirm('Mark this order as Completed?')) return;
                                    this.isLoading = true;
                                    
                                    fetch('../update_status.php', {
                                        method: 'POST',
                                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                                        body: 'id=<?= $order['id'] ?>&status=Completed'
                                    })
                                    .then(res => res.json())
                                    .then(data => {
                                        if(data.success) {
                                            this.status = 'Completed';
                                        } else {
                                            alert('Error: ' + (data.message || 'Unknown error'));
                                        }
                                    })
                                    .catch(err => alert('Network Error'))
                                    .finally(() => this.isLoading = false);
                                },
                                inviteToRepo() {
                                    if(!this.github) return alert('No GitHub username provided.');
                                    if(!confirm('Invite ' + this.github + ' to the repository?')) return;
                                    this.isInviting = true;
                                    
                                    fetch('invite_github.php', {
                                        method: 'POST',
                                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                                        body: 'username=' + encodeURIComponent(this.github) + '&order_id=<?= $order['id'] ?>'
                                    })
                                    .then(res => res.json())
                                    .then(data => {
                                        if(data.status === 'success') {
                                            alert('Invitation sent successfully!');
                                        } else {
                                            alert('Error: ' + (data.message || 'Unknown error'));
                                        }
                                    })
                                    .catch(err => alert('Network Error'))
                                    .finally(() => this.isInviting = false);
                                },
                                processDelivery() {
                                    if(!confirm('Confirm payment and grant GitHub access for this order?')) return;
                                    this.isLoading = true;
                                    
                                    // 1. Mark as Completed
                                    fetch('../update_status.php', {
                                        method: 'POST',
                                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                                        body: 'id=<?= $order['id'] ?>&status=Completed'
                                    })
                                    .then(res => res.json())
                                    .then(data => {
                                        if(data.success) {
                                            this.status = 'Completed';
                                            // 2. Grant GitHub Access
                                            if(this.github) {
                                                return fetch('invite_github.php', {
                                                    method: 'POST',
                                                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                                                    body: 'username=' + encodeURIComponent(this.github) + '&order_id=<?= $order['id'] ?>'
                                                });
                                            }
                                        } else {
                                            throw new Error(data.message || 'Failed to update status');
                                        }
                                    })
                                    .then(res => res ? res.json() : null)
                                    .then(data => {
                                        if(data && data.status === 'success') {
                                            alert('Payment confirmed and invitation(s) sent!');
                                        } else if(data) {
                                            alert('Payment confirmed, but GitHub invitation failed: ' + data.message);
                                        } else if(this.status === 'Completed') {
                                            alert('Payment confirmed. (No GitHub username to invite)');
                                        }
                                    })
                                    .catch(err => alert('Error: ' + err.message))
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
                                    <div class="flex flex-col">
                                        <div class="font-medium text-gray-800"><?= htmlspecialchars($order['customer']['name']) ?></div>
                                        <div class="text-xs text-gray-400 mb-1"><?= htmlspecialchars($order['customer']['email']) ?></div>
                                        <?php if (!empty($github)): ?>
                                            <div class="text-[10px]">
                                                <a href="https://github.com/<?= htmlspecialchars($github) ?>" target="_blank" class="text-blue-500 hover:underline">
                                                    <i class="fa-brands fa-github"></i> @<?= htmlspecialchars($github) ?>
                                                </a>
                                            </div>
                                        <?php endif; ?>
                                        <div class="mt-2 text-[10px] text-gray-500 bg-gray-50 p-1 rounded border">
                                            <ul class="list-disc ml-3">
                                                <?php 
                                                $items = $order['details']['cart'] ?? [];
                                                foreach ($items as $item): ?>
                                                    <li><?= htmlspecialchars($item['name']) ?> (x<?= $item['quantity'] ?>)</li>
                                                <?php endforeach; ?>
                                            </ul>
                                        </div>
                                    </div>
                                </td>
                                <td class="py-3 px-6">
                                    <template x-if="status === 'Pending'">
                                        <span class="badge badge-warning gap-2">
                                            Pending
                                        </span>
                                    </template>
                                    <template x-if="status === 'Completed' || status === 'Shipped'">
                                        <span class="badge badge-success text-white gap-2">
                                            Completed
                                        </span>
                                    </template>
                                </td>
                                <td class="py-3 px-6">
                                    $<?= number_format((float)$total, 2) ?>
                                </td>
                                <td class="py-3 px-6 text-right">
                                    <a 
                                        x-show="status === 'Completed' || status === 'Shipped' || true" 
                                        href="<?= isset($order['invoice_file']) ? '../view_invoice.php?file=' . htmlspecialchars($order['invoice_file']) : '#' ?>" 
                                        target="_blank"
                                        class="btn btn-xs btn-outline btn-info mr-2"
                                        :class="{ 'btn-disabled': !'<?= isset($order['invoice_file']) ? $order['invoice_file'] : '' ?>' }"
                                    >
                                        <i class="fa-solid fa-file-pdf"></i> PDF
                                    </a>
                                    <button 
                                        x-show="status === 'Pending'"
                                        @click="processDelivery()" 
                                        :disabled="isLoading"
                                        class="btn btn-xs btn-outline btn-success"
                                    >
                                        <span x-show="isLoading" class="loading loading-spinner loading-xs"></span>
                                        <i x-show="!isLoading" class="fa-solid fa-check-double mr-1"></i>
                                        Confirm Payment & Grant Access
                                    </button>
                                    <button 
                                        x-show="status !== 'Pending'"
                                        @click="inviteToRepo()" 
                                        :disabled="isInviting || !github"
                                        class="btn btn-xs btn-outline btn-primary ml-1"
                                    >
                                        <span x-show="isInviting" class="loading loading-spinner loading-xs"></span>
                                        <i x-show="!isInviting" class="fa-brands fa-github mr-1"></i>
                                        Invite
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
