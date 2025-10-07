using Microsoft.EntityFrameworkCore;

namespace generator_web.Models
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options)
            : base(options) { }

        public DbSet<generator_data> generator_datas { get; set; }
        public DbSet<User> Users { get; set; }
        public DbSet<Alert> Alerts { get; set; }
        public DbSet<ControlAction> ControlActions { get ; set; }

    }
}
