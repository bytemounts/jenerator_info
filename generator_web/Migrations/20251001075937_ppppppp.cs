using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace generator_web.Migrations
{
    /// <inheritdoc />
    public partial class ppppppp : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "Alerts",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    Type = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Message = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    CreatedAt = table.Column<DateTime>(type: "datetime2", nullable: false),
                    IsActive = table.Column<bool>(type: "bit", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Alerts", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "ControlActions",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    userName = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    ActionType = table.Column<string>(type: "nvarchar(50)", maxLength: 50, nullable: false),
                    Description = table.Column<string>(type: "nvarchar(200)", maxLength: 200, nullable: false),
                    Timestamp = table.Column<DateTime>(type: "datetime2", nullable: false),
                    IpAddress = table.Column<string>(type: "nvarchar(45)", maxLength: 45, nullable: false),
                    IsExecuted = table.Column<bool>(type: "bit", nullable: false),
                    Result = table.Column<string>(type: "nvarchar(500)", maxLength: 500, nullable: false),
                    ExecutedAt = table.Column<DateTime>(type: "datetime2", nullable: true),
                    Status = table.Column<string>(type: "nvarchar(20)", maxLength: 20, nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_ControlActions", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "generator_datas",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    CalismaDurumu = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    OperationMode = table.Column<int>(type: "int", nullable: false),
                    SistemCalismaSuresi = table.Column<int>(type: "int", nullable: false),
                    SebekeVoltaj_l1 = table.Column<int>(type: "int", nullable: false),
                    SebekeVoltaj_l2 = table.Column<int>(type: "int", nullable: false),
                    SebekeVoltaj_l3 = table.Column<int>(type: "int", nullable: false),
                    SebekeHz = table.Column<int>(type: "int", nullable: false),
                    ToplamGuc = table.Column<int>(type: "int", nullable: false),
                    SebekeDurumu = table.Column<bool>(type: "bit", nullable: false),
                    GenVoltaj_l1 = table.Column<int>(type: "int", nullable: false),
                    GenVoltaj_l2 = table.Column<int>(type: "int", nullable: false),
                    GenVoltaj_l3 = table.Column<int>(type: "int", nullable: false),
                    GenHz = table.Column<int>(type: "int", nullable: false),
                    GenUretilenGuc = table.Column<int>(type: "int", nullable: false),
                    GenGucFaktoru = table.Column<int>(type: "int", nullable: false),
                    MotorRpm = table.Column<int>(type: "int", nullable: false),
                    MotorSicaklik = table.Column<int>(type: "int", nullable: false),
                    YagBasinci = table.Column<int>(type: "int", nullable: false),
                    YakitSeviyesi = table.Column<int>(type: "int", nullable: false),
                    BataryaVoltaji = table.Column<int>(type: "int", nullable: false),
                    timestamp = table.Column<int>(type: "int", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_generator_datas", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "Users",
                columns: table => new
                {
                    UserId = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    Username = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    email = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    PasswordHash = table.Column<string>(type: "nvarchar(max)", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Users", x => x.UserId);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "Alerts");

            migrationBuilder.DropTable(
                name: "ControlActions");

            migrationBuilder.DropTable(
                name: "generator_datas");

            migrationBuilder.DropTable(
                name: "Users");
        }
    }
}
