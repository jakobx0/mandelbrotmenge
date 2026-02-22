vim.opt.makeprg = "make build"
vim.opt.errorformat = "%f:%l:%c: %t%*[^:]: %m,%f:%l: %t%*[^:]: %m,%f:%l:%c: %m,%f:%l: %m"

vim.keymap.set("n", "<leader>mb", "<cmd>make build<cr>", { desc = "Build Mandelbrot" })
vim.keymap.set("n", "<leader>ms", "<cmd>!make run-server WIDTH=1280 HEIGHT=720 PORT=5000<cr>", { desc = "Run server" })
vim.keymap.set("n", "<leader>mc", "<cmd>!make run-client HOST=127.0.0.1 PORT=5000<cr>", { desc = "Run client" })
